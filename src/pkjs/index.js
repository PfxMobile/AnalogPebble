var DEBUG = 0;
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var myAPIKey = '590387ffcb577045625c7d3fe07d7ea0';

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url, false);
  xhr.send();
};

// function locationSuccess(pos) {
//   // Construct URL
//   var weatherUrl = 'https://api.darksky.net/forecast/' + myAPIKey + '/' + pos.coords.latitude + ',' + pos.coords.longitude;
  
//   // get forecast through dark sky
//   xhrRequest(weatherUrl, 'GET', 
//     function(responseText) {
//       var json = JSON.parse(responseText);
      
//       // round temperature
//       var temp = Math.round(json.currently.temperature);
//       console.log("Temperature is " + temp);
      
//       // icon for weather condition
//       var icon = json.currently.icon;
//       console.log("Current icon is " + icon);
      
//       // assemble dictionary using keys
//       var dictionary = {
//         "KEY_TEMP": temp,
//         "KEY_ICON": icon,
//       };
      
//       // Send to Pebble
//       Pebble.sendAppMessage(dictionary,
//         function(e) {
//           console.log("Weather info sent to Pebble successfully!");
//         },
//         function(e) {
//           console.log("Error sending weather info to Pebble!");
//         }
//       );
//     }      
//   );
// }

function locationSuccess(pos) {
  // parse config
  var settings = {};
      settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var unit=(settings.KEY_WEATHER_UNIT==1) ? 'imperial' : 'metric';  
  // Construct URL
  var weatherUrl = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + myAPIKey + '&units=' + unit;
     
  // Send request to OpenWeatherMap.org
  xhrRequest(weatherUrl, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);
            
      // Delivered in imperial so no need for adjustment
      var temp = Math.round(json.main.temp);
      if (DEBUG) console.log("Temperature is " + temp); 
      
      var wind = Math.round(json.wind.speed);
      if (DEBUG) console.log("Wind is " + wind); 

      var deg = Math.round(json.wind.deg);
      var wind_dir = 'n';
      if (DEBUG) console.log("Wind deg is " + deg);
      if ( deg <= 45 ) {
          wind_dir = 'n';
      } else if ( deg < 135 ) {
          wind_dir = 'e';
      } else if ( deg < 225 ) {
          wind_dir = 's';
      } else if ( deg < 315 ) {
          wind_dir = 'w';
      };
      if (DEBUG) console.log("Wind direction is " + wind_dir);

      var icon = json.weather[0].icon;
      if (DEBUG) console.log("Icon is " + icon);
      
      // assemble dictionary using keys
      var dictionary = {
        "KEY_TEMP": temp,
        "KEY_ICON": icon,
        "KEY_WIND": wind,
        "KEY_DEG" : wind_dir,
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          if (DEBUG) console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          if (DEBUG) console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  if (DEBUG) console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    if (DEBUG) console.log("PebbleKit JS ready!");
/* only on request from event!
    var settings = {};
      settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    if (settings.KEY_WEATHER_UPDATE_INTERVAL != 0 ) {
    // Get the initial weather
    getWeather();
    }
*/
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    if (DEBUG) console.log("AppMessage received!");
    getWeather();
  }                     
);
