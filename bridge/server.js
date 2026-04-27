const WebSocket = require("ws");
const net = require("net");

const wss = new WebSocket.Server({ port: 3000 });

wss.on("connection", ws => {
 const client = new net.Socket();
 client.connect(8080,"127.0.0.1");

 ws.on("message", msg => client.write(msg));
 client.on("data", data => ws.send(data.toString()));
});
