const Discord = require('discord.js');
const discordBot = new Discord.Client();
const net = require('net');
const buffer = require('buffer');

const CLIENT_CONNECTED = '0';
const NAME_CHANGED = '1';
const NEW_MESSAGE = '2';
const CHAT_WHOIS = '3';

const CHANNEL_ID = "YOUR_CHANNEL_ID";
const BOT_TOKEN = "YOUR_BOT_TOKEN"

discordBot.on('ready', () => {
  var channel = discordBot.channels.get(CHANNEL_ID);
  console.log('CONNECTED TO DISCORD')
  var clients = [];

  // Send a message to all clients
  function broadcast(message, sender) {
    for (var i = 0; i < clients.length; i++) {
      var client = clients[i];
      if (client !== sender) {
        try {
          client.write(new Buffer(message, 'ascii'));
        } catch(e) {console.error(e)}
      }
    }
  }

  function removeColors(string) {
    return string.replace(/\^[0-9]/g, "");
  }

  function dropSocket(socket) {
    //channel.sendMessage('**[JK] '+removeColors(socket.name)+'** s\'est déconnecté.');
    broadcast('<'+socket.name+'^7> s\'est déconnecté.', socket);
    clients.splice(clients.indexOf(socket), 1);
  }

  net.createServer(function (socket) {
    clients.push(socket);

    // Handle incoming messages from clients.
    socket.on('data', function (data) {
      data = buffer.transcode(data, 'binary','utf8').toString();
      var type = data.charAt(0);
      var message = data.substring(1);
      var messageConverted = removeColors(message);
      if (type === CLIENT_CONNECTED) {
        //channel.sendMessage('**[JK] '+messageConverted+'** vient de se connecter.');
        broadcast('<'+message+'^7> vient de se connecter.', socket);
        socket.name = message;
      }
      else if (type === NAME_CHANGED) {
        //if (removeColors(socket.name) != messageConverted) channel.sendMessage('**[JK] '+removeColors(socket.name)+'** s\'est renommé en **'+messageConverted+'**.');
        broadcast('<'+socket.name+'^7> s\'est renommé en <'+message+'>.');
        socket.name = message;
      }
      else if (type === NEW_MESSAGE) {
        channel.sendMessage('**[JK] '+removeColors(socket.name)+':** '+messageConverted);
        broadcast('^9[AFJ] ^7'+socket.name+'^7: '+message, socket);
      }
      else if (type === CHAT_WHOIS) {
        var whois = '^3[AFJ] ^7Bot: ^3Connectés [ ^7';
        for (var i = 0; i < clients.length; i++) {
          whois += (i==0?'':'^3, ^7')+clients[i].name;
        }
        whois += "^3 ]"
        socket.write(new Buffer(whois, 'ascii'))
      }
      else {
        console.error("UNHANDLED TYPE: "+type);
      }
    });

    // Remove the client from the list when it leaves
    socket.on('end', function () {
      dropSocket(socket);
    });

    socket.on("error", (err) => {
      dropSocket(socket);
    })

  }).listen(29069);



  discordBot.on('message', message => {
    // Server commands
    if (message.content.startsWith('!whois')) {
      var newMessage = '**Connectés :**\n\`\`\`';
      for (var i = 0; i < clients.length; i++) {
        newMessage += '\n '+removeColors(clients[i].name);
      }
      newMessage += '\n\`\`\`';
      message.author.sendMessage(newMessage);
    }
    // No more commands, send the message
    else if (message.author !== discordBot.user) broadcast('^8[AFJ] ^7'+message.author.username+'^7: '+message.content);
  });
});

discordBot.login(BOT_TOKEN);
