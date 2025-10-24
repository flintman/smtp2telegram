# SMTP-2-Telegram

SMTP-2-Telegram is a lightweight C++ service that listens for incoming emails via small SMTP server and forwards their contents to a specified Telegram chat using a bot. This is useful for receiving notifications, alerts, or any email-based messages directly in Telegram.  This is a project of mine as I work for a company with an old ems system that can't communicate with newer smtp servers and we lost the ability to get emails.   This is a great little work around and figured I'd share.

## Features

- Receives emails via SMTP
- Forwards email subject and body to Telegram chat
- Configurable via environment variables
- Installs as a systemd service via .deb package
- Automatically creates `.env` file if not present
- Stores all files (binary, config, logs) in `~/smtp2telegram/`
- Lightweight and easy to deploy

## Requirements

- C++17 compatible compiler
- Make (for building, if not using .deb)
- Telegram Bot Token
- Telegram Chat ID

## Installation

### Using .deb Package

1. Download and install the .deb package:
    ```bash
    sudo dpkg -i smtp2telegram_VERSION_ARCH.deb
    ```

2. The systemd service will be enabled automatically.

3. See configuration below.

### Manual Build

1. Required dependencies:
    ```bash
    sudo apt install g++ libboost-all-dev libcurl4-openssl-dev
    ```

2. Clone the repository:
    ```bash
    git clone https://github.com/flintman/smtp2telegram.git
    cd smtp2telegram
    ```

3. Build the project deb:
    ```bash
    make deb
    ```

## Configuration

On first run, the service will create a `.env` file in `~/smtp2telegram/` if it does not exist by asking
you the questions for each variable.

Set the following environment variables in `~/smtp2telegram/.env` manually if you like:

| Variable              | Description                                      |
|-----------------------|--------------------------------------------------|
| `API_KEY`             | Token for your Telegram bot                      |
| `CHAT_ID`             | Chat ID to send messages to                      |
| `SMTP_HOSTNAME`       | Host/IP to listen for SMTP (default: `0.0.0.0`)  |
| `SMTP_PORT`           | Port to listen for SMTP (default: `1025`)        |
| `LOG_KEEP_DAYS`       | Days to keep logs (default: `3`)                 |

Example `~/smtp2telegram/.env` file:
```env
API_KEY=123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11
CHAT_ID=-1001234567890
SMTP_HOSTNAME=0.0.0.0
SMTP_PORT=1025
LOG_KEEP_DAYS=3
```

## Usage

First Run(*** TO CREATE THE .env FILE follow directions***):
```bash
smtp2telegram
```

Start the service (if not already running and have .env file):
```bash
sudo systemctl start smtp2telegram
```

Send an email to the configured SMTP server. The service will forward the email's subject and body to the specified Telegram chat.

Logs are saved to `~/smtp2telegram/smtp2telegram.log`.

## License

MIT License

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## Contact

For questions or support, open an issue on GitHub.

