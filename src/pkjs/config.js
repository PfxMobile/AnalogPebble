module.exports = [
	{ 
    	"type": "heading", 
    	"defaultValue": "Settings",
		  "size": 1
	},
	{
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Color Selection"
			},
			{
				"type": "toggle",
				"messageKey": "KEY_INVERT_COLORS",
				"label": "Invert Colors",
				"defaultValue": true 
			}
		]
	},
    {
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Regional Settings"
			},
			{
  				"type": "select",
  				"messageKey": "KEY_LANG",
  				"defaultValue": "1",
  				"label": "Language",
  				"options": [
    					{ 
      						"label": "EN", 
      						"value": "0"
    					},
    					{ 
      						"label": "RU",
      						"value": "1" 
    					},
				]
            },
			{
  				"type": "select",
  				"messageKey": "KEY_DATE_FORMAT",
  				"defaultValue": "%d",
  				"label": "Date format",
  				"options": [
                        { 
      						"label": "Day", 
      						"value": "%d"
    					},
    					{ 
      						"label": "Day-Month", 
      						"value": "%d-%m"
    					},
    					{ 
      						"label": "Month-Day",
      						"value": "%m-%d" 
    					},
				]
            },
            {
  				"type": "select",
  				"messageKey": "KEY_WEATHER_UNIT",
  				"defaultValue": "0",
  				"label": "Weather units",
  				"options": [
    					{ 
      						"label": "Metric (Celsius, meter/sec)", 
      						"value": "0"
    					},
    					{ 
      						"label": "Imperial (Fahrenheit, miles/hour)",
      						"value": "1" 
    					},
				]
            }
		]
    },
    {
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Other Settings"
			},
			{
  				"type": "select",
  				"messageKey": "KEY_VIBRATE_INTERVAL",
  				"defaultValue": "0",
  				"label": "Auto vibrate interval",
  				"options": [
    					{ 
      						"label": "OFF", 
      						"value": "0"
    					},
    					{ 
      						"label": "10 min",
      						"value": "10" 
    					},
    					{ 
      						"label": "15 min",
      						"value": "15" 
    					},
    					{ 
      						"label": "20 min",
      						"value": "20" 
    					},
    					{ 
      						"label": "30 min",
      						"value": "30" 
    					},
    					{ 
      						"label": "hour",
      						"value": "60" 
    					},
				]
            },
			{
  				"type": "select",
  				"messageKey": "KEY_WEATHER_UPDATE_INTERVAL",
  				"defaultValue": "30",
  				"label": "Weather update interval",
  				"options": [
    					{ 
      						"label": "OFF (disable weather)", 
      						"value": "0"
    					},
    					{ 
      						"label": "5 min", 
      						"value": "5"
    					},
    					{ 
      						"label": "10 min", 
      						"value": "10"
    					},
    					{ 
      						"label": "30 min (save battery)",
      						"value": "30" 
    					},
    					{ 
      						"label": "1 hour",
      						"value": "60" 
    					},
    					{ 
      						"label": "4 hour",
      						"value": "240" 
    					},
    					{ 
      						"label": "8 hour",
      						"value": "480" 
    					},
				]
            },
			{
				"type": "toggle",
				"messageKey": "KEY_DRAW_ALL_NUMBERS",
				"label": "Draw all numbers (1-12). Only for rectangle display",
				"defaultValue": true 
			},
            {
                                "type": "toggle",
                                "messageKey": "KEY_VIBRATE_ON_BT_LOST",
                                "label": "Vibrate if Bluetooth lost",
                                "defaultValue": true
            },
		]
    },
	{
		"type": "submit",
		"defaultValue": "Apply Settings"
	},
		{ "type": "text","defaultValue": "This project is free and OpenSource. You can support it by donate. Thank you!<br><a  href='https://paypal.me/stosem'><img alt='Donate with PayPal.me' style='border-width:0' src='data:image/png;base64,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' /></a> - [ paypal.me/stosem ]"
  },
  {
    "type": "text",
    "defaultValue": "<a href='http://openweathermap.org/'>Weather provided by Open Weather Map</a>"
  },

];
