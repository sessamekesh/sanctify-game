const withTM = require("next-transpile-modules")(["@sanctify/pve-offline-wasm"]);

module.exports = withTM({
  webpackDevMiddleware: config => {
    config.watchOptions = {
      poll: 1000,
      aggregateTimeout: 300,
    }
    return config
  },
});
