import React from 'react';
import './App.scss';

const TIME_START = 7;
const TIME_END = 21;

const range = (start, stop) =>
  Array(Math.ceil((stop - start))).fill(start).map((x, y) => x + y)

class Grid extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      times: range(TIME_START, TIME_END)
    }
  }

  getPos(n) {
    return {
      left: `${((n - TIME_START) / (TIME_END - TIME_START)) * 100}%`
    };
  }

  render() {
    return (
      <span className="grid">
        {
          this.state.times.map(n => (
            <div className="grid__line" style={this.getPos(n)} key={n}>
              <span className="grid__line__text">{n}</span>
            </div>
          ))
        }
      </span>
    )
  }
}

class App extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      tables: []
    }
  }

  componentDidMount() {
    fetch('./timetables.json')
      .then(x => x.json())
      .then(x => this.setState({tables: x}));
  }

  formatTime(t) {
    return `${t.hours}`.padStart(2, '0') + ':' + `${t.minutes}`.padStart(2, '0');
  }

  getPos(et) {
    const totalWidthMinutes = (TIME_END - TIME_START) * 60;

    const startMinutes = ((et.from.hours - TIME_START) * 60) + et.from.minutes;
    const endMinutes = ((et.to.hours - TIME_START) * 60) + et.to.minutes;
    const widthMinutes = endMinutes - startMinutes;

    return {
      width: `${(widthMinutes / totalWidthMinutes) * 100}%`,
      left: `${(startMinutes / totalWidthMinutes) * 100}%`,
    }
  }

  render() {
    return (
      <>
        <h1>TTGen</h1>
        <div className="tables">
          {
            this.state.tables.map((t, i) => (
              <div className="tables__content" key={i}>
                <h2 className="tables__content__title" >TimeTable #{i + 1}</h2>
                <div className="tables__content__table">
                  <Grid />
                  {
                    t.days.map((d, j) => (
                      <div className="tables__content__table__day" key={j}>
                        {
                          d.map((e, k) => (
                            <div className={`tables__content__table__day__event tables__content__table__day__event--${e.type}`} style={this.getPos(e.time)} key={k}>
                              <span className="tables__content__table__day__event__name">{e.name}</span>
                              <span className="tables__content__table__day__event__time">{this.formatTime(e.time.from)} - {this.formatTime(e.time.to)}</span>
                              <span className="tables__content__table__day__event__parallel">Par: {e.parallel}</span>
                            </div>
                          ))
                        }
                      </div>
                    ))
                  }
                </div>
              </div>
            ))
          }
        </div>
      </>
    );
  }
}

export default App;
