/**
 * Copies this project to the Raspberry Pi via rsync.
 * Create a JSON file named "sync.config.json" that has these properties:
 *  - username
 *  - hostname
 *  - directory
 *  - quiet (optional)
 */
import { directory, hostname, quiet, username } from "./sync.config.json";

import chalk from "chalk";
import Rsync from "rsync";

// Build the command
const rsync = Rsync.build({
  shell: "ssh",
  flags: "ahP",
  recursive: true,
  source: ["./build/clock", "./src/module/fonts", "./config.json"],
  destination: `${username}@${hostname}:${directory}`,
});

console.log(chalk.magenta(`\n🚀\t$ ${rsync.command()}`));

// Execute the command
rsync
  .output(
    (data) =>
      quiet ||
      console.log(
        chalk.blue(`📤\t${data.toString().split("\n").slice(0, 1).join("")}`),
      ),
    (stderr) =>
      quiet ||
      console.error(
        chalk.red(`📤\t${stderr.toString().split("\n").slice(0, 1).join("")}`),
      ),
  )
  .execute((error, code) => {
    if (error) {
      console.error(chalk.red("👎\t", error));
    } else {
      console.log(chalk.green(`👍\tDone! [exit code ${code}]\n\n`));

      // Copy config.json to /etc for external configuration
      console.log(chalk.magenta("📋\tCopying config.json to /etc/clock-config.json..."));
      const { execSync } = require("child_process");
      const { readFileSync } = require("fs");
      try {
        const configContent = readFileSync("./config.json", "utf-8");
        const escapedConfig = configContent.replace(/'/g, "'\\''");
        execSync(
          `ssh ${username}@${hostname} "echo '${escapedConfig}' > /etc/clock-config.json"`,
          { stdio: "inherit" },
        );
        console.log(chalk.green("✓\tConfig copied to /etc/clock-config.json"));
      } catch (err) {
        console.error(chalk.red("✗\tFailed to copy config to /etc:"), err);
      }
    }
  });
