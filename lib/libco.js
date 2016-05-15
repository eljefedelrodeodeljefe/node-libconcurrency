class Lib {
  constructor() {
    global.go = this.go
  }
}

Lib.prototype.go = () => {
  return 0
};

module.exports = new Lib()
