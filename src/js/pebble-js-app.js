var CLEAR_DAY = 0;
var CLEAR_NIGHT = 1;
var WINDY = 2;
var COLD = 3;
var PARTLY_CLOUDY_DAY = 4;
var PARTLY_CLOUDY_NIGHT = 5;
var HAZE = 6;
var CLOUD = 7;
var RAIN = 8;
var SNOW = 9;
var HAIL = 10;
var CLOUDY = 11;
var STORM = 12;
var FOG = 13;
var NA = 14;

var imageId = {
  0 : STORM, //tornado
  1 : STORM, //tropical storm
  2 : STORM, //hurricane
  3 : STORM, //severe thunderstorms
  4 : STORM, //thunderstorms
  5 : HAIL, //mixed rain and snow
  6 : HAIL, //mixed rain and sleet
  7 : HAIL, //mixed snow and sleet
  8 : COLD, //freezing drizzle
  9 : RAIN, //drizzle
  10 : COLD, //freezing rain
  11 : RAIN, //showers
  12 : RAIN, //showers
  13 : SNOW, //snow flurries
  14 : SNOW, //light snow showers
  15 : SNOW, //blowing snow
  16 : SNOW, //snow
  17 : HAIL, //hail
  18 : RAIN, //sleet
  19 : HAZE, //dust
  20 : FOG, //foggy
  21 : HAZE, //haze
  22 : HAZE, //smoky
  23 : WINDY, //blustery
  24 : WINDY, //windy
  25 : COLD, //cold
  26 : CLOUDY, //cloudy
  27 : PARTLY_CLOUDY_NIGHT, //mostly cloudy (night)
  28 : CLOUDY, //mostly cloudy (day)
  29 : PARTLY_CLOUDY_NIGHT, //partly cloudy (night)
  30 : PARTLY_CLOUDY_DAY, //partly cloudy (day)
  31 : CLEAR_NIGHT, //clear (night)
  32 : CLEAR_DAY, //sunny
  33 : CLEAR_NIGHT, //fair (night)
  34 : PARTLY_CLOUDY_DAY, //fair (day)
  35 : HAIL, //mixed rain and hail
  36 : CLEAR_DAY, //hot
  37 : STORM, //isolated thunderstorms
  38 : STORM, //scattered thunderstorms
  39 : STORM, //scattered thunderstorms
  40 : RAIN, //scattered showers
  41 : SNOW, //heavy snow
  42 : SNOW, //scattered snow showers
  43 : SNOW, //heavy snow
  44 : CLOUD, //partly cloudy
  45 : STORM, //thundershowers
  46 : SNOW, //snow showers
  47 : STORM, //isolated thundershowers
  3200 : NA, //not available
};

var options = JSON.parse(localStorage.getItem('options'));
//console.log('read options: ' + JSON.stringify(options));
if (options === null) options = { "use_gps" : "true",
                                  "location" : "",
                                  "units" : "fahrenheit",
                                  "invert_color" : "false",
								  "bluetoothvibe" : "false",
								  "hidesec" : "false",
								  "background" : "false",
								  "background2" : "false",
								  "background3" : "false",
								  "background4" : "false",
								  "background5" : "false",
								  "date2" : "false",
								  "hidew" : "false",
								  "bigtext" : "false",
								  "hidebt" : "false",
								  "hidebatt" : "false",
								  "hourlyvibe" : "false"};

function getWeatherFromLatLong(latitude, longitude) {
  var response;
  var woeid = -1;
  var query = encodeURI("select woeid from geo.placefinder where text=\""+latitude+","+longitude + "\" and gflags=\"R\"");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          woeid = response.query.results.Result.woeid;
          getWeatherFromWoeid(woeid);
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
}

function getWeatherFromLocation(location_name) {
  var response;
  var woeid = -1;

  var query = encodeURI("select woeid from geo.places(1) where text=\"" + location_name + "\"");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        // console.log(req.responseText);
        response = JSON.parse(req.responseText);
        if (response) {
          woeid = response.query.results.place.woeid;
          getWeatherFromWoeid(woeid);
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
}

function getWeatherFromWoeid(woeid) {
  var celsius = options['units'] == 'celsius';
  var query = encodeURI("select item.condition from weather.forecast where woeid = " + woeid +
                        " and u = " + (celsius ? "\"c\"" : "\"f\""));
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";

  var response;
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          var condition = response.query.results.channel.item.condition;
          temperature = condition.temp + (celsius ? "\u00B0" : "\u00B0");
          icon = imageId[condition.code];
          // console.log("temp " + temperature);
          // console.log("icon " + icon);
          // console.log("condition " + condition.text);
          Pebble.sendAppMessage({
            "icon" : icon,
            "temperature" : temperature,
            "invert_color" : (options["invert_color"] == "true" ? 1 : 0),
			"bluetoothvibe" : (options["bluetoothvibe"] == "true" ? 1 : 0),
            "hidesec" : (options["hidesec"] == "true" ? 1 : 0),
            "background" : (options["background"] == "true" ? 1 : 0),
            "background2" : (options["background2"] == "true" ? 1 : 0),
            "background3" : (options["background3"] == "true" ? 1 : 0),
            "background4" : (options["background4"] == "true" ? 1 : 0),
            "background5" : (options["background5"] == "true" ? 1 : 0),
            "date2" : (options["date2"] == "true" ? 1 : 0),
            "hidew" : (options["hidew"] == "true" ? 1 : 0),
            "bigtext" : (options["bigtext"] == "true" ? 1 : 0),
            "hidebt" : (options["hidebt"] == "true" ? 1 : 0),
            "hidebatt" : (options["hidebatt"] == "true" ? 1 : 0),
            "hourlyvibe" : (options["hourlyvibe"] == "true" ? 1 : 0),
		  });
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
}

function updateWeather() {
  if (options['use_gps'] == "true") {
    window.navigator.geolocation.getCurrentPosition(locationSuccess,
                                                    locationError,
                                                    locationOptions);
  } else {
    getWeatherFromLocation(options["location"]);
  }
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

function locationSuccess(pos) {
  var coordinates = pos.coords;
  getWeatherFromLatLong(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "icon":11,
    "temperature":"   "
  });
}

Pebble.addEventListener('showConfiguration', function(e) {
  var uri = 'http://www.themapman.com/pebblewatch/mystyle3.html?' +
    'use_gps=' + encodeURIComponent(options['use_gps']) +
    '&location=' + encodeURIComponent(options['location']) +
    '&units=' + encodeURIComponent(options['units']) +
    '&invert_color=' + encodeURIComponent(options['invert_color']) +
    '&bluetoothvibe=' + encodeURIComponent(options['bluetoothvibe']) +
	'&hourlyvibe=' + encodeURIComponent(options['hourlyvibe']) +
	'&hidesec=' + encodeURIComponent(options['hidesec']) +
    '&background=' + encodeURIComponent(options['background']) +
    '&background2=' + encodeURIComponent(options['background2']) +
    '&background3=' + encodeURIComponent(options['background3']) +
    '&background4=' + encodeURIComponent(options['background4']) +
    '&background5=' + encodeURIComponent(options['background5']) +
    '&date2=' + encodeURIComponent(options['date2']) +
    '&hidew=' + encodeURIComponent(options['hidew']) +
    '&bigtext=' + encodeURIComponent(options['bigtext']) +
    '&hidebt=' + encodeURIComponent(options['hidebt']) +
    '&hidebatt=' + encodeURIComponent(options['hidebatt']);
	//console.log('showing configuration at uri: ' + uri);

  Pebble.openURL(uri);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    options = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem('options', JSON.stringify(options));
    //console.log('storing options: ' + JSON.stringify(options));
    updateWeather();
  } else {
    console.log('no options received');
  }
});

Pebble.addEventListener("ready", function(e) {
  //console.log("connect!" + e.ready);
  updateWeather();
  setInterval(function() {
    //console.log("timer fired");
    updateWeather();
  }, 1200000); // 20 minutes
  console.log(e.type);
});
