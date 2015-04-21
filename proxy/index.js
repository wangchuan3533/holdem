// Setup basic express server
var express = require('express');
var app = express();
var server = require('http').createServer(app);
var io = require('socket.io')(server);
var port = process.env.PORT || 8899;
var net = require('net');
var md5 = require('MD5');

server.listen(port, function () {
  console.log('Server listening at port %d', port);
});

// Routing
app.use(express.static(__dirname + '/public'));

io.on('connection', function (socket) {
  socket.on('message', function (data) {
    if (socket.peer) {
      socket.peer.write(data.toString() + "\r\n");
    }
  });

  // when the client emits 'join', this listens and executes
  socket.on('join', function (user) {
    username = user.username;
    password = md5(user.password);
    method   = user.method;

    peer = net.connect({port:10000}, function() {
      console.log(username + ' connected to peer server!');
      peer.write("type 1" + "\r\n");
      peer.write(method + ' '  + username + ' ' + password + "\r\n");
    });
    
    peer.on('data', function(resp) {
      lines = resp.toString().split("\n");
      for (i = 0; i < lines.length; i++) {
        if (lines[i].length) {
          try {
            var msg = JSON.parse(lines[i]);
            console.log(msg);
            if (msg && msg.type && msg.data) {
              socket.emit(msg.type, msg.data);
            }
          } catch (err) {
            console.log(lines[i]);
            socket.emit('message', {
                'username' : '',
                'message'  : lines[i]
            });
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
