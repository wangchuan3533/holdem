$(function() {
  var FADE_TIME = 150; // ms
  var TYPING_TIMER_LENGTH = 400; // ms
  var COLORS = [
    '#e21400', '#91580f', '#f8a700', '#f78b00',
    '#58dc00', '#287b00', '#a8f07a', '#4ae8c4',
    '#3b88eb', '#3824aa', '#a700ff', '#d300e7'
  ];

  // Initialize varibles
  var $window = $(window);
  var $usernameInput = $('.usernameInput'); // Input for username
  var $userpassInput = $('.userpassInput'); // Input for password
  var $loginButton = $('.loginButton'); // login button
  var $registerButton = $('.registerButton'); // register button
  var $messages = $('.messages'); // Messages area
  var $inputMessage = $('.inputMessage'); // Input message input box
  var $loginPage = $('.login.page'); // The login page
  var $chatPage = $('.main.page'); // The chatroom page
  var $betButton = $('.controlArea input[value="bet"]');
  var $checkButton = $('.controlArea input[value="check"]');
  var $raiseButton = $('.controlArea input[value="raise"]');
  var $callButton = $('.controlArea input[value="call"]');
  var $foldButton = $('.controlArea input[value="fold"]');
  var $allInButton = $('.controlArea input[value="all_in"]');
  var $rangeControl = $('.controlArea input[type="range"]');
  var $rangeValue = $('.controlArea .rangeValue');

  // Prompt for setting a username
  var username;
  var connected = false;
  var typing = false;
  var lastTypingTime;
  var player_index = -1;

  var socket = io();

  // user login or register
  function userLogin (method) {
    username = cleanInput($usernameInput.val().trim());
    password = cleanInput($userpassInput.val().trim());

    // If the username is valid
    if (method && username && password) {
      // Tell the server your username
      socket.emit('join', {'method': method, 'username': username, 'password': password});
    }
  }

  // Sends a chat message
  function sendMessage () {
    var message = $inputMessage.val();
    // Prevent markup from being injected into the message
    message = cleanInput(message);
    // if there is a non-empty message and a socket connection
    if (message && connected) {
      $inputMessage.val('');
      addChatMessage({
        username: username,
        message: message
      });
      // tell server to execute 'new message' and send along one parameter
      socket.emit('message', message);
    }
  }

  // Log a message
  function log (message, options) {
    var $el = $('<li>').addClass('log').text(message);
    addMessageElement($el, options);
  }

  // Adds the visual chat message to the message list
  function addChatMessage (data, options) {

    var $usernameDiv = $('<span class="username"/>')
      .text(data.username)
      .css('color', getUsernameColor(data.username));
    var $messageBodyDiv = $('<span class="messageBody">')
      .text(data.message);

    var typingClass = data.typing ? 'typing' : '';
    var $messageDiv = $('<li class="message"/>')
      .data('username', data.username)
      .addClass(typingClass)
      .append($usernameDiv, $messageBodyDiv);

    addMessageElement($messageDiv, options);
  }

  // Adds a message element to the messages and scrolls to the bottom
  // el - The element to add as a message
  // options.fade - If the element should fade-in (default = true)
  // options.prepend - If the element should prepend
  //   all other messages (default = false)
  function addMessageElement (el, options) {
    var $el = $(el);

    // Setup default options
    if (!options) {
      options = {};
    }
    if (typeof options.fade === 'undefined') {
      options.fade = true;
    }
    if (typeof options.prepend === 'undefined') {
      options.prepend = false;
    }

    // Apply options
    if (options.fade) {
      $el.hide().fadeIn(FADE_TIME);
    }
    if (options.prepend) {
      $messages.prepend($el);
    } else {
      $messages.append($el);
    }
    $messages[0].scrollTop = $messages[0].scrollHeight;
  }

  // Prevents input from having injected markup
  function cleanInput (input) {
    return $('<div/>').text(input).text();
  }

  // Gets the color of a username through our hash function
  function getUsernameColor (username) {
    // Compute hash code
    var hash = 7;
    for (var i = 0; i < username.length; i++) {
       hash = username.charCodeAt(i) + (hash << 5) - hash;
    }
    // Calculate color
    var index = Math.abs(hash % COLORS.length);
    return COLORS[index];
  }

  // login
  $loginButton.click(function() {
    userLogin('login');
  });

  // register
  $registerButton.click(function() {
    userLogin('reg');
  });

  $window.keydown(function (event) {

    // When the client hits ENTER on their keyboard
    if (event.which === 13) {
      if (username) {
        sendMessage();
      }
    }
  });

  // Focus input when clicking on the message input's border
  $inputMessage.click(function () {
    $inputMessage.focus();
  });

  // Socket events

  // Whenever the server emits 'login', log the login message
  socket.on('login', function (data) {
    $loginPage.fadeOut();
    $chatPage.show();
    $loginPage.off('click');
    connected = true;
    log(data.message, {
      prepend: true
    });
  });

  socket.on('player', function (player) {
    player_index = player.player;
    $('.controlArea .index').text(player_index);
  });

  // Whenever the server emits 'new message', update the chat body
  socket.on('message', function (data) {
    console.log(data);
    log(data.message, {
    });
    //addChatMessage(data);
  });

  socket.on('tables', function (data) {
    console.log(data);
    $('.hallArea .tables .table').remove();
    for (i = 0; i < data.tables.length; i++) {
      $table = $('<input type="button" class="table" />').prop('value', data.tables[i]);
      $table.click(function () {
        socket.emit('message', 'join ' + this.value);
      });
      $('.hallArea .tables').append($table);
    }
  });

  socket.on('users', function (data) {
    console.log(data);
    $('.hallArea .users').text(data.users);
  });

  socket.on('cards', function (data) {
    if (data.cards && data.cards.length) {
      $('.controlArea .cards .card1').remove();
      $('.controlArea .cards .card2').remove();
      $('.controlArea .cards .card3').remove();
      $('.controlArea .cards .card4').remove();
      for (i = 0; i < data.cards.length; i++) {
        $card = $('<span/>').text(data.cards[i]);
        if (data.cards[i].search('♥')  >= 0) {
            $card.addClass('card1');
        } else if (data.cards[i].search('♦') >= 0) {
            $card.addClass('card2');
        } else if (data.cards[i].search('♠') >= 0) {
            $card.addClass('card3');
        } else if (data.cards[i].search('♣') >= 0) {
            $card.addClass('card4');
        }
        $('.controlArea .cards').append($card);
      }
    }


  });

  socket.on('table', function (table) {
    console.log(table);
    $('.tableArea .name').text(table.name);
    $('.tableArea .chips').text(table.chips);
    $('.tableArea .bet').text(table.bet);
    $('.tableArea .pot').text(table.pot);
    $('.playersArea .player').removeClass('turn');
    $('.controlArea input[type="button"]').prop('disabled', true);
    if (player_index == table.turn) {
      for (i = 0; i < table.actions.length; i++) {
        $('.controlArea input[value="' + table.actions[i] + '"]').prop('disabled', false);
      }
    }

    if (table.cards && table.cards.length) {
      $('.tableArea .cards .card1').remove();
      $('.tableArea .cards .card2').remove();
      $('.tableArea .cards .card3').remove();
      $('.tableArea .cards .card4').remove();
      for (i = 0; i < table.cards.length; i++) {
        $card = $('<span/>').text(table.cards[i]);
        if (table.cards[i].search('♥')  >= 0) {
            $card.addClass('card1');
        } else if (table.cards[i].search('♦') >= 0) {
            $card.addClass('card2');
        } else if (table.cards[i].search('♠') >= 0) {
            $card.addClass('card3');
        } else if (table.cards[i].search('♣') >= 0) {
            $card.addClass('card4');
        }
        $('.tableArea .cards').append($card);
      }
    }

    if (table.players && table.players.length) {
      for (i = 0; i < table.players.length; i++) {
        player = table.players[i];
        if (i == table.turn) {
          $('.playersArea .player').eq(i).addClass('turn');
          $rangeValue.text(table.min);
          $rangeControl.prop('min', table.min);
          $rangeControl.prop('max', player.chips);
        }
        $('.playersArea .player').eq(i).find('.index').text(player.player);
        $('.playersArea .player').eq(i).find('.name').text(player.name);
        $('.playersArea .player').eq(i).find('.chips').text(player.chips);
        $('.playersArea .player').eq(i).find('.bet').text(player.bet);
        if (player.cards) {
          $('.playersArea .player').eq(i).find('.cards').text(player.cards);
        }
        if (player.hand) {
          $('.playersArea .player').eq(i).find('.hand').text(player.hand);
        }
      }
    }
  });

  $betButton.off().click(function() {
    socket.emit('message', 'bet ' + $rangeControl.prop('value'));
  });

  $raiseButton.off().click(function() {
    socket.emit('message', 'raise ' + $rangeControl.prop('value'));
  });

  $checkButton.off().click(function() {
    socket.emit('message', 'check');
  });

  $foldButton.off().click(function() {
    socket.emit('message', 'fold');
  });

  $callButton.off().click(function() {
    socket.emit('message', 'call');
    console.log('call');
  });

  $allInButton.off().click(function() {
    socket.emit('message', 'all in');
  });

  $rangeControl.on('change', function() {
    $rangeValue.text($rangeControl.val());
  });
});
