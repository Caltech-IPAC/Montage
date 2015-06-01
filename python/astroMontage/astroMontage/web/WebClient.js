function WebClient(port)
{
   var me = this;

   me.debug = true;

   me.msgCallbacks = [];



   // Start up the connection and set up
   // callbacks for open, close, and 
   // message receive.

   me.host   = "ws://localhost:" + port + "/ws";

   if(me.debug)
      console.log("DEBUG> host: " + me.host);   
    
   me.socket = new WebSocket(me.host);

   if(me.debug)
      console.log("DEBUG> socket status: " + me.socket.readyState);   
    
   if(me.socket)
   {
      // Open callback

      me.socket.onopen = function()
      {
         if(me.debug)
            console.log("DEBUG>  Connection opened ...");
      }


      // Receive message callback

      me.socket.onmessage = function(msg)
      {
          me.receive(msg.data);
      }


      // Close callback

      me.socket.onclose = function()
      {
        if(me.debug)
           console.log("DEBUG>  The connection has been closed.");
      }
   }
   else
      if(me.debug)
         console.log("DEBUG>  Invalid socket.");



   // SEND to server

   me.send = function(text)
   {
      if(text == "")
         return;
      
      if(me.debug)
         console.log("DEBUG>  Sending: " + text);

      me.socket.send(text);
   }


   // RECEIVE from server

   me.receive = function(msg)
   {
      if(me.debug)
         console.log("DEBUG>  Receiving: " + msg);

      for(i=0; i<me.msgCallbacks.length; ++i)
         me.msgCallbacks[i](msg);
   } 


   me.addMsgCallback = function(method)
   {
      me.msgCallbacks.push(method);
   }
}
