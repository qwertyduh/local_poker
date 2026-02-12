# 🃏 Local Poker Betting Platform

A lightweight, local-network betting synchronization tool built in C++ using Raylib. This project allows multiple laptops on the same Wi-Fi network to track a central "Pot" in real-time while playing physical card games.

## 🚀 Features
- **Real-time Synchronization:** Bets made on one laptop update the pot on all connected screens instantly.
- **Native macOS Performance:** Built with C++ and Raylib for a smooth, hardware-accelerated UI.
- **Cross-Laptop Communication:** Uses TCP/IP sockets to link "Players" to a central "Banker."

---

## 🛠 Prerequisites

### For the Developer (To Compile)
You need the Raylib library installed via Homebrew:
```bash
brew install raylib
