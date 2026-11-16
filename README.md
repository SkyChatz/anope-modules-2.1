# bs_rps — Rock-Paper-Scissors for Anope BotServ

A module for [Anope](https://anope.org/) IRC services that lets users play
Rock-Paper-Scissors against a channel's assigned BotServ bot, either via
`/msg BotServ` or the in-channel fantasy command `!rps`.

---

## Features

- **Play against the channel bot** — Rock, Paper, or Scissors.
- **Transparent results** — the bot announces both its own move and the
  outcome (e.g. *"I chose Paper, you chose Rock. Paper wraps Rock."*).
- **Move validation** — invalid moves are rejected with a clear message
  instead of being silently treated as a tie.
- **Rate limiting** — a 2-second per-nick cooldown prevents channel spam.
- **Table-driven outcomes** — easy to extend and maintain.
- **Localized strings** — all user-facing text is wrapped with `_()` for
  translation.
- **Dual invocation** — works both as a BotServ command and as a fantasy
  command in-channel.

---

## Requirements

- **Anope 2.1 or newer** (tested on **2.1.27**)
- An IRC daemon supported by Anope (tested with **UnrealIRCd 6+**)
- A C++11 (or later) compiler toolchain

---

## Installation

1. **Copy the module source** into your Anope build tree:

   ```bash
   cp bs_rps.cpp /path/to/anope-2.1.xx/modules/third/bs_rps.cpp

## Rebuild and install:

bash
cd /path/to/anope-2.1.xx/build
make bs_rps
make install
Enable the module in your services.conf:

## conf
module
{
    name = "bs_rps"
}

command
{
    service = "BotServ"
    name    = "RPS"
    command = "botserv/rps"
}

fantasy
{
    name            = "RPS"
    command         = "botserv/rps"
    prepend_channel = true
}
Restart Anope (or reload if the module supports it).
