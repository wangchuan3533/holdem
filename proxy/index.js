// Setup basic express server
var express = require('express');
var app = express();
var server = require('http').createServer(app);
var io = require('socket.io')(server);
var port = process.env.PORT || 8899;
var net = require('net');

server.listen(port, function () {
  console.log('Server listening at port %d', port);
});

// Routing
app.use(express.static(__dirname + '/public'));

io.on('connection', function (socket) {
  socket.on('message', function (data) {
    if (socket.peer) {
      socket.peer.write(data.toString() + "\n");
    }
  });

  // when the client emits 'join', this listens and executes
  socket.on('join', function (user) {
    username = user.username;
    password = user.password;
    method   = user.method;

    peer = net.connect({port:10000}, function() {
      console.log(username + ' connected to peer server!');
      peer.write("type 1\n");
      peer.write(method + ' '  + username + ' ' + password + "\n");
    });
    
    peer.on('data', function(resp) {
      lines = resp.toString().split("\n");
      for (i = 0; i < lines.length; i++) {
        if (lines[i].length) {
          var msg = JSON.parse(lines[i]);
          console.log(msg);
          if (msg && msg.type && msg.data) {
            socket.emit(msg.type, msg.data);
          }
        }
      }
    });
    socket.peer = peer;
  });

  // when the user disconnects.. perform this
  socket.on('disconnect', function () {
    // disconnect with the peer
    if (socket.peer) {
      socket.peer.destroy()
    }
  });
});
