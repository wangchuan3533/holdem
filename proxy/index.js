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

// Chatroom

// usernames which are currently connected to the chat
var usernames = {};
var numUsers = 0;

io.on('connection', function (socket) {
  var addedUser = false;
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

    // we store the username in the socket session for this client
    socket.username = username;
    // add the client's username to the global list
    usernames[username] = username;
    ++numUsers;
    addedUser = true;
    socket.emit('login', {
      numUsers: numUsers
    });

    socket.broadcast.emit('join', {
      username: socket.username,
      numUsers: numUsers
    });

    peer = net.connect({port:10000}, function() {
      console.log(username + ' connected to peer server!');
      peer.write(method + ' '  + username + ' ' + password + "\n");
    });
    
    peer.on('data', function(resp) {
      lines = resp.toString().split("\n");
      for (i = 0; i < lines.length; i++) {
        if (lines[i].length) {
          socket.emit('message', {
            username: '',
            message: lines[i]
          });
        }
      }
    });
    socket.peer = peer;
  });

  // when the client emits 'typing', we broadcast it to others
  socket.on('typing', function () {
    socket.broadcast.emit('typing', {
      username: socket.username
    });
  });

  // when the client emits 'stop typing', we broadcast it to others
  socket.on('typing_off', function () {
    socket.broadcast.emit('typing_off', {
      username: socket.username
    });
  });

  // when the user disconnects.. perform this
  socket.on('disconnect', function () {
    // disconnect with the peer
    if (socket.peer) {
      socket.peer.destroy()
    }
    // remove the username from global usernames list
    if (addedUser) {
      delete usernames[socket.username];
      --numUsers;

      // echo globally that this client has left
      socket.broadcast.emit('quit', {
        username: socket.username,
        numUsers: numUsers
      });
    }
  });
});
