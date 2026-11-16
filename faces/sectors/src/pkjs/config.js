module.exports = [
  { "type": "heading", "defaultValue": "Sectors" },
  { "type": "text", "defaultValue": "Nine dots make ten minutes. Ten minutes fill a sector. Three sectors make a half." },

  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Colours" },
    { "type": "radiogroup", "messageKey": "Theme", "label": "Theme", "defaultValue": 0,
      "options": [
        { "label": "Dark", "value": 0 },
        { "label": "Light", "value": 1 }
      ] },
    { "type": "color", "messageKey": "AccentColor", "label": "Accent", "defaultValue": "FFAA00", "sunlight": true },
    { "type": "toggle", "messageKey": "LowBatteryRed", "label": "Accent turns red when battery is low", "defaultValue": true }
  ] },

  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Hour numeral" },
    { "type": "radiogroup", "messageKey": "Numeral", "label": "Style", "defaultValue": 0,
      "options": [
        { "label": "Bold", "value": 0 },
        { "label": "Geometric", "value": 1 },
        { "label": "Dots", "value": 2 }
      ] }
  ] },

  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Sectors" },
    { "type": "toggle", "messageKey": "Seams", "label": "Seams between finished sectors", "defaultValue": true },
    { "type": "toggle", "messageKey": "FillClockwise", "label": "Fill dots clockwise (instead of ring by ring)", "defaultValue": false }
  ] },

  { "type": "submit", "defaultValue": "Save" }
];
