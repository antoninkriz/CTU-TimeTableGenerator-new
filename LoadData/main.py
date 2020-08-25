import sys
import requests
import json
from datetime import datetime, time
from pprint import PrettyPrinter

pp = PrettyPrinter(width=160)
pprint = pp.pprint

"""
USER
"""

CLIENT_ID = sys.argv[1]
CLIENT_SECRET = sys.argv[2]
CLIENT = (CLIENT_ID, CLIENT_SECRET)

FACULTY = int(sys.argv[3])
SEMESTER = sys.argv[4]
COURSES = sys.argv[5].split(' ')

"""
AUTH
"""

# Consts
AUTH_URL = 'https://auth.fit.cvut.cz/oauth/token'
AUTH_DATA = {'grant_type': 'client_credentials'}

# Get OAuth token
token_response = requests.post(AUTH_URL, auth=CLIENT, data=AUTH_DATA)
token = token_response.json()['access_token']

"""
SIRIUS
"""

# Consts
EVEN = 'even'
ODD = 'odd'
EVEN_ODD = [EVEN, ODD]
SIRIUS_URL = 'https://sirius.fit.cvut.cz/api/v1'
SIRIUS_URL_SEMESTERS = f'/faculties/{FACULTY}/semesters'
SIRIUS_URL_WEEKS = f'/faculties/{FACULTY}/schedule/weeks'
SIRIUS_URL_COURSES = {course: f'/courses/{course}/events' for course in COURSES}

# Sirius session
s = requests.Session()
s.headers.update({'Authorization': f'Bearer {token}'})

# Get semester
res_semesters = s.get(f'{SIRIUS_URL}/{SIRIUS_URL_SEMESTERS}')
semester = next((s for s in list(res_semesters.json()['semesters']) if s['semester'] == SEMESTER), None)

if semester is None:
    raise Exception('Semester not found')

PARITY = {k: 0 if semester['first_week_parity'] == k else 5 for k in EVEN_ODD}

# Get weeks
res_weeks = s.get(f'{SIRIUS_URL}/{SIRIUS_URL_WEEKS}', params={
    'from': semester['starts_at'],
    'to': semester['ends_at']
})
weeks_all = list(res_weeks.json()['semester_weeks'])
weeks = (w for w in weeks_all if w['period_types'] == ['teaching'])
week = {k: next((w for w in weeks if w['week_parity'] == k), None) for k in EVEN_ODD}

if week[EVEN] is None:
    raise Exception('No even teaching week found')

if week[ODD] is None:
    raise Exception('No odd teaching week found')

# Get classes
classes = {
    course: {
        'lecture': [],
        'tutorial': [],
        'laboratory': []
    } for course in COURSES
}

for course in COURSES:
    res_course = {
        k: s.get(f'{SIRIUS_URL}{SIRIUS_URL_COURSES[course]}', params={
            'limit': 1000,
            'from': week[k]['starts_at'],
            'to': week[k]['ends_at']
        }) for k in EVEN_ODD
    }

    events = {k: list(res_course[k].json()['events']) for k in EVEN_ODD}

    for k in events:
        for e in events[k]:
            classes[course][e['event_type']].append({
                'time': {
                    'week': k,
                    'day': PARITY[k] + datetime.fromisoformat(e['starts_at']).weekday(),
                    'from': datetime.fromisoformat(e['starts_at']).time(),
                    'to': datetime.fromisoformat(e['ends_at']).time()
                },
                'name': e['links']['course'],
                'parallel': e['parallel']
            })

# Merge parallels
for course in classes:
    for event_type in classes[course]:
        grouped = {}

        for e in classes[course][event_type]:
            if e['parallel'] not in grouped:
                grouped[e['parallel']] = []
            grouped[e['parallel']].append(e)

        classes[course][event_type] = [grouped[p] for p in grouped]

# Remove duplicate parallels
for course in classes:
    for event_type in classes[course]:
        seen = []
        clean = []

        for parallel in classes[course][event_type]:
            prop = [e['time'] for e in parallel]

            if prop not in seen:
                clean.append(parallel)
                seen.append(prop)

        classes[course][event_type] = clean


def json_serial(x):
    if isinstance(x, (time, datetime)):
        return f'{str(x.hour).zfill(2)}:{str(x.minute).zfill(2)}'
    else:
        raise TypeError(f'Type {type(x)} not serializable')


result = json.dumps(classes, default=json_serial)
print(result)
