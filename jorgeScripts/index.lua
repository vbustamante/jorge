local header = [[
HTTP/1.1 200 OK
Server: Jorge/0.2
Content-Length: $bodyLen
Content-Type: text/html

]]

local body = [[
<!DOCTYPE html>
<html>
  <head>
    <title>Jorge Root</title>
    <script
      src="https://code.jquery.com/jquery-3.2.1.min.js"
      integrity="sha256-hwg4gsxgFZhOsEEamdOYGBf13FyQuiTwlAQgxVSNgt4="
      crossorigin="anonymous"></script>

    <script>
      $(document).ready(function(){
        var lastMessageId = 0;
        var userName = prompt("Enter username");

        $("input[type=text]").focus();

        $("form").submit(function(e){
          e.preventDefault();
          var $this = $(this);
          var msg = $this.children("input[type=text]").val();

          $this.children("input[type=submit]").attr("disabled", true)

          $.post($this.attr("action"), {"msg":msg, "sender":userName})
            .done(function(data){
                $this.children("input[type=text]").val("");
                $("ul").append("<li class='adhoc'>"+(userName||"anon")+" said: " + msg)
            })
            .always(function(){
                $this.children("input[type=submit]").attr("disabled", false)
            });
          });

          function updates(){
            $.post({
                url: "/getMessages.lua",
                data: {"id":lastMessageId+1},
                dataType: 'json'
            })
            .done(function(data){
                var msgs = data.messages;
                $ul = $("ul");
                $(".adhoc").remove();
                for(var i=0; i<msgs.length; i++){
                    var m = msgs[i];
                    $ul.append("<li>"+(m.sender!='NULL'?m.sender:"anon")+" said: " + m.msg);
                    if(lastMessageId < m.id) lastMessageId = m.id;
                }
            })
            .fail(function(err){
                console.log("err");
                console.log(err);
            })
            .always(function(){
                setTimeout(updates, 1000);
            });
          }

          updates();

      });
    </script>
  </head>
  <body>
    <h1>Welcome to the World</h1>
    <ul>
    </ul>
    <form type="POST" action="/sendMessage.lua">
      <input type="text" name="msg" placeholder="Digite sua mensagem">
      <input type="submit">
    </form>
  </body>
</html>
]]

local bodyLen = echo(body)


local header = header:gsub('\n', '\r\n'):gsub('$bodyLen', bodyLen)
local headerLen = setHeader(header)

-- print("Lua - "..bodyLen+headerLen.." bytes sent, H:"..headerLen.." & B:"..bodyLen)
