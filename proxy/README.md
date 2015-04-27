
# Texas Hold'em Poker websocket proxy

Texas Hold'em Poker websocket proxy

## How to use

```
$ cd proxy
$ npm install
$ cd public
$ bower install
$ cd ..
$ node index
```

And point your browser to `http://localhost:8899`. Optionally, specify
a port by supplying the `PORT` env variable.

## Features

- Multiple users can join a chat room by each entering a unique username
on website load.
- Users can type chat messages to the chat room.
- A notification is sent to all users when a user joins or leaves
the chatroom.
