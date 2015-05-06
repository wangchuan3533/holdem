var app = angular.module('texasApp', ['btford.socket-io', 'ui.bootstrap', 'ui.slider']);
app.factory('socket', function(socketFactory) {
  return socketFactory();
});

app.controller('TableController', function($scope, socket) {
  $scope.username  = '';
  $scope.password  = '';
  $scope.alert     = {};
  $scope.connected = false;
  $scope.joined    = false;
  $scope.new_table = '';
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
  $scope.state = 0;
  $scope.community_cards = [];
  $scope.players = [];
  $scope.action = {};
  $scope.positions = ['positions0', 'position1', 'position2', 'position3', 'position4', 'position5', 'position6', 'position7', 'position8', 'position9'];
  $scope.range = {'min': 100, 'max': 1000, 'value': 100, 'step': 100};

  $scope.login = function(method) {
    // If the username is valid
    if (method && $scope.username && $scope.password) {
      // Tell the server your username
      socket.emit('join', {'method': method, 'username': $scope.username, 'password': $scope.password});
    }
  };

  $scope.keydown = function(event) {
    // When the client hits ENTER on their keyboard
    if (event.which === 13) {
      if ($scope.connected && $scope.join && $scope.inputMessage) {
        socket.emit('message', $scope.inputMessage);
        $scope.inputMessage = '';
      }
    }
  };

  $scope.join = function(name) {
    console.log(name);
    socket.emit('message', 'join ' + name);
  };

  $scope.quit = function() {
    socket.emit('message', 'quit');
  };

  $scope.create_table = function(name) {
    socket.emit('message', 'mk ' + name);
    $scope.new_table = '';
  }

  $scope.game_start = function() {
    socket.emit('message', 'start');
  }

  $scope.game_action = function(action, value) {
    switch (action) {
    case 'bet':
    case 'raise':
      socket.emit('message', action + ' ' + value);
      break;
    case 'call':
    case 'fold':
    case 'check':
      socket.emit('message', action);
      break;
    case 'all_in':
      socket.emit('message', 'all in');
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
    if (data.player >= 0) {
      $scope.joined = true;
    } else {
      $scope.joined = false;
    }
  });

  socket.on('message', function(data) {
    console.log(data);
    if (!$scope.connected) {
      $scope.alert = {type: 'danger', msg: data.message};
    }
    $scope.messages.unshift(data.message);
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
    $scope.state = data.state;
    $scope.community_cards = data.cards;
    $scope.players = data.players;
    $scope.actions = ($scope.player_index >= 0 && $scope.player_index == data.turn) && data.actions;
    $scope.range = {'min': 100, 'max': 1000, 'value': 100};
  });
});
