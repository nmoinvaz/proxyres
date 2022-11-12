const { spawn } = require("child_process");
const { platform } = require("os");

if (platform() === "win32") {
    spawn("yarn", ["run", "dhcp_server"], { stdio: "inherit" });
} else {
    spawn("sudo", ["yarn", "run", "dhcp_server"], { stdio: "inherit" });
}
