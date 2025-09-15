# SMTP-2-Telegram

SMTP-2-Telegram is a lightweight C++ service that listens for incoming emails via SMTP and forwards their contents to a specified Telegram chat using a bot. This is useful for receiving notifications, alerts, or any email-based messages directly in Telegram.

## Features

- Receives emails via SMTP
- Forwards email subject and body to Telegram chat
- Configurable via environment variables
- Lightweight and easy to deploy

## Requirements

- C++17 compatible compiler
- Make (for building)
- Telegram Bot Token
- Telegram Chat ID

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/flintman/smtp2telegram.git
    cd smtp-2-telegram
    ```

2. Build the project:
    ```bash
    make
    ```

## Configuration

Set the following environment variables before running the service:

| Variable              | Description                                      |
|-----------------------|--------------------------------------------------|
| `API_KEY`             | Token for your Telegram bot                      |
| `CHAT_ID`             | Chat ID to send messages to                      |
| `SMTP_HOSTNAME`       | Host/IP to listen for SMTP (default: `0.0.0.0`)  |
| `SMTP_PORT`           | Port to listen for SMTP (default: `1025`)        |

Example `.env` file:
```env
TELEGRAM_BOT_TOKEN=123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11
TELEGRAM_CHAT_ID=-1001234567890
SMTP_LISTEN_HOST=0.0.0.0
SMTP_LISTEN_PORT=1025
```

## Usage

Start the service:
```bash
./smtp2telegram
```

Send an email to the configured SMTP server. The service will forward the email's subject and body to the specified Telegram chat.

## License

MIT License

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## Contact

For questions or support, open an issue on GitHub.

