const http = require('http')
const fs = require('fs')
const socketIo = require('socket.io')
const { Client } = require('pg')
const redis = require('redis')

const port = 3000
const htmlContent = fs.readFileSync('index.html')
const hostname = '0.0.0.0'
const redisClient = redis.createClient()

const pgClient = new Client({
    user: 'pranaybansal', // Use your Mac username
    host: 'localhost',
    database: 'local_poker', // The DB we created in psql
    password: '',            // Usually blank for local dev
    port: 5432,
});
pgClient.connect();
redisClient.connect();

// 1. creating a server that writes the html
const server = http.createServer((req,res) => {
    res.writeHead(200, { 'Content-Type': 'text/html' });
    res.end(htmlContent);
})

// 2. init socket.io and passing the server var
const io = socketIo(server);

// 3. creating a connection
io.on('connection', (socket) => {
    let customId = socket.handshake.query.customId;
    if (customId === "Anon") {
        customId = `Anon_${socket.id.substring(0, 5)}`; 
    }
    socket.customId = customId;

    console.log(`User connected: ${socket.customId}`);

    // 4. Listen for a custom event from the user
    socket.on('update_number', async (data) => {
        console.log(`A:${socket.id} User ${socket.customId} sent number: ${data.userValue}`)

        await redisClient.set(`live_val:${socket.id}`, data.userValue);
        await redisClient.hSet('active_users', data.userName, data.userValue)
        // 5. Broadcast this to EVERYONE
        io.emit('number_changed', {
            userName: data.userName,
            userValue: data.userValue
        });
    });

    socket.on('disconnect', async () => {
        const finalValue = await redisClient.get(`live_val:${socket.id}`);

        if (finalValue && !socket.customId.startsWith("Anon_")) {
            const query = `
                INSERT INTO hand_history (player_name, amount) 
                VALUES ($1, $2)
                ON CONFLICT (player_name) 
                DO UPDATE SET 
                    amount = EXCLUDED.amount,
                    timestamp = CURRENT_TIMESTAMP;
            `;
            // We'll log 'Final State' as the action
            await pgClient.query(query, [socket.customId, parseInt(finalValue)]);
            
            // 5. Clean up Redis
            await redisClient.del(`live_val:${socket.id}`);
            console.log(`Archived data for ${socket.customId} to Postgres.`);}
    });
});

server.listen(port, hostname, () => {
    console.log(`Server is accessible at http://192.169.14.227:${port}`);
})