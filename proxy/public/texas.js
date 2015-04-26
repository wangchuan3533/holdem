var app = angular.module('texasApp', ['btford.socket-io']);
app.factory('socket', function(socketFactory) {
  return socketFactory();
});

app.controller('TableController', function($scope, socket) {
  $scope.username  = '';
  $scope.password  = '';
  $scope.notice    = '';
  $scope.connected = false;
  $scope.inputMessage = '';
  $scope.messages = [];
  $scope.player_index = -1;
  $scope.tables = [];
  $scope.users = [];
  $scope.cards = [];
  $scope.name = '';
  $scope.bet  = 0;
  $scope.pot  = 0;
  $scope.turn = -1;
  $scope.community_cards = [];
  $scope.players = [];
  $scope.action = {};

  this.login = function(method) {
    // If the username is valid
    if (method && $scope.username && $scope.password) {
      // Tell the server your username
      socket.emit('join', {'method': method, 'username': $scope.username, 'password': $scope.password});
    }
  };

  this.keydown = function(event) {
    // When the client hits ENTER on their keyboard
    if (event.which === 13) {
      if ($scope.connected && $scope.inputMessage) {
        socket.emit('message', $scope.inputMessage);
        $scope.inputMessage = '';
      }
    }
  };

  this.join = function(name) {
    console.log(name);
    socket.emit('message', 'join ' + name);
  };

  this.game_action = function(action, value) {
    switch (action) {
    case 'bet':
    case 'raise':
      socket.emit('message', action + ' ' + value);
      break;
    case 'call':
    case 'fold':
    case 'check':
    case 'all_in':
      socket.emit('message', action);
      break;
    default:
      console.log(action);
      break;
    }
  };

  socket.on('login', function(data) {
    console.log(data);
    $scope.connected = true;
  });

  socket.on('player', function(data) {
    console.log(data);
    $scope.player_index = data.player;
  });

  socket.on('message', function(data) {
    console.log(data);
    if (!$scope.connected) {
      $scope.notice = data.message;
    }
    $scope.messages.push(data.message);
  });

  socket.on('tables', function(data) {
    console.log(data);
    $scope.tables = data.tables;
  });

  socket.on('users', function(data) {
    console.log(data);
    $scope.users = data.users;
  });

  socket.on('cards', function(data) {
    console.log(data);
    $scope.cards = data.cards;
  });

  socket.on('table', function(data) {
    console.log(data);
    $scope.name = data.name;
    $scope.bet  = data.bet;
    $scope.pot  = data.pot;
    $scope.turn = data.turn;
    $scope.community_cards = data.cards;
    $scope.players = data.players;
    $scope.actions = ($scope.player_index >= 0 && $scope.player_index == data.turn) && data.actions;
    $scope.range = {'min': 100, 'max': 1000, 'value': 100};
  });
});
