const { spawn } = require("child_process");

spawn("yarn", ["run", "http_server"], { stdio: "inherit" });
