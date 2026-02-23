# 🃏 Local Poker: Real-Time Sync & Persistence

A professional-grade real-time web application that synchronizes user data across multiple screens using a high-performance "Hot/Cold" data architecture.

---

## 🏗️ The Tech Stack

| Layer | Technology | Purpose |
| :--- | :--- | :--- |
| **Real-Time** | **Socket.io** | Bi-directional event-based communication (The "Live Pipe"). |
| **Hot Storage** | **Redis** | In-memory key-value store for high-speed "live" value updates. |
| **Cold Storage** | **PostgreSQL** | Relational database for persistent player profiles and session logs. |
| **Backend** | **Node.js** | The event-driven "Dispatcher" managing the data flow. |
| **Frontend** | **HTML5/JS** | Client-side UI that interacts via custom WebSocket events. |

---

## ⚙️ Installation & Setup

### 1. Database Engines
Ensure you have the following services running on your Mac:
```bash
# Start PostgreSQL (Homebrew)
brew services start postgresql@15

# Start Redis (Homebrew)
brew services start redis
```
## Now open  postgres to create a database
```
psql -d postgres
```
# Inside psql:
```
CREATE DATABASE local_poker;
\c local_poker;

CREATE TABLE hand_history (
    id SERIAL PRIMARY KEY,
    player_name TEXT UNIQUE NOT NULL, 
    amount INTEGER,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```
# Crucial: Add the unique constraint for the Upsert logic
```
ALTER TABLE hand_history ADD CONSTRAINT unique_player_name UNIQUE (player_name);

npm install socket.io pg redis
```
## How to run
start the server
```
node app.js
```
now connect to your localhost:3000 or ipv4:3000 and enjoy
