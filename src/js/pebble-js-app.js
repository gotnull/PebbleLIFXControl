Pebble.addEventListener("ready",
    function(e) {
        console.log("App is ready.");
    }
);

// Gets the current price list from Mt. Gox.
function activateSwitch(setting) {
    console.log("Turning " + setting + " light(s).");

    var response;
    var req = new XMLHttpRequest();

    var data = {
      "data": setting,
      "result": "success"
    };

    req.open("POST", "http://192.168.2.56:8045/RubiksService.svc/exec", true);
    req.setRequestHeader("Content-type", "application/json");
    
	req.onload = function(e) {
		console.log("Response received.");
		if (req.readyState == 4) {
			if (req.status == 200) {
				console.log("Received response from web service:");

				response = JSON.parse(req.responseText);
				//console.log(response);

				if (response.result == "success") {
					data = JSON.stringify(response);
					console.log(data);
				} else {
					console.log("API didn't return success. Received " + response.success.toString() + " instead.");
				}
          } else {
              console.log("HTTP status returned was not 200. Received " + req.status.toString() + " instead.");
          }
      } else {
          console.log("Didn't receieve ready status of 4. Received " + req.readyState.toString() + " instead.");
      }
    };
    req.send(JSON.stringify(data));
}

Pebble.addEventListener("appmessage",
    function(e) {
		console.log("Received command from Pebble:");
		console.log(JSON.stringify(e.payload[1]));
		
		var switchSetting = e.payload[1];
		activateSwitch(switchSetting);
	}
);
