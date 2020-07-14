# Tempertature WiFi IoT

[![Build Status](https://travis-ci.org/educonz/jardineiro-wifi-iot.svg?branch=master)](https://travis-ci.org/educonz/jardineiro-wifi-iot)

### Configuração

Alterar nome e senha WiFI.

```sh
const char *ssid = "YOUR_WIFI";
const char *password = "YOUR_PASS";
```

Alterar token para integrar com [S.IoT](https://www.konztec.com).

```sh
const char *token = "";
const char *endPointSiot = "https://test.ws.siot.com/api/v1";
```