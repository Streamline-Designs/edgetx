/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

struct CrossfireSensor {
  const uint8_t id;
  const uint8_t subId;
  const char * name;
  const TelemetryUnit unit;
  const uint8_t precision;
};

enum CrossfireSensorIndexes {
  RX_RSSI1_INDEX,
  RX_RSSI2_INDEX,
  RX_QUALITY_INDEX,
  RX_SNR_INDEX,
  RX_ANTENNA_INDEX,
  RF_MODE_INDEX,
  TX_POWER_INDEX,
  TX_RSSI_INDEX,
  TX_QUALITY_INDEX,
  TX_SNR_INDEX,
  BATT_VOLTAGE_INDEX,
  BATT_CURRENT_INDEX,
  BATT_CAPACITY_INDEX,
  GPS_LATITUDE_INDEX,
  GPS_LONGITUDE_INDEX,
  GPS_GROUND_SPEED_INDEX,
  GPS_HEADING_INDEX,
  GPS_ALTITUDE_INDEX,
  GPS_SATELLITES_INDEX,
  ATTITUDE_PITCH_INDEX,
  ATTITUDE_ROLL_INDEX,
  ATTITUDE_YAW_INDEX,
  FLIGHT_MODE_INDEX,
  UNKNOWN_INDEX,
};

const CrossfireSensor crossfireSensors[] = {
  {LINK_ID,        0, ZSTR_RX_RSSI1,    UNIT_DB,            0},
  {LINK_ID,        1, ZSTR_RX_RSSI2,    UNIT_DB,            0},
  {LINK_ID,        2, ZSTR_RX_QUALITY,  UNIT_PERCENT,       0},
  {LINK_ID,        3, ZSTR_RX_SNR,      UNIT_DB,            0},
  {LINK_ID,        4, ZSTR_ANTENNA,     UNIT_RAW,           0},
  {LINK_ID,        5, ZSTR_RF_MODE,     UNIT_RAW,           0},
  {LINK_ID,        6, ZSTR_TX_POWER,    UNIT_MILLIWATTS,    0},
  {LINK_ID,        7, ZSTR_TX_RSSI,     UNIT_DB,            0},
  {LINK_ID,        8, ZSTR_TX_QUALITY,  UNIT_PERCENT,       0},
  {LINK_ID,        9, ZSTR_TX_SNR,      UNIT_DB,            0},
  {BATTERY_ID,     0, ZSTR_BATT,        UNIT_VOLTS,         1},
  {BATTERY_ID,     1, ZSTR_CURR,        UNIT_AMPS,          1},
  {BATTERY_ID,     2, ZSTR_CAPACITY,    UNIT_MAH,           0},
  {GPS_ID,         0, ZSTR_GPS,         UNIT_GPS_LATITUDE,  0},
  {GPS_ID,         0, ZSTR_GPS,         UNIT_GPS_LONGITUDE, 0},
  {GPS_ID,         2, ZSTR_GSPD,        UNIT_KMH,           1},
  {GPS_ID,         3, ZSTR_HDG,         UNIT_DEGREE,        3},
  {GPS_ID,         4, ZSTR_ALT,         UNIT_METERS,        0},
  {GPS_ID,         5, ZSTR_SATELLITES,  UNIT_RAW,           0},
  {ATTITUDE_ID,    0, ZSTR_PITCH,       UNIT_RADIANS,       3},
  {ATTITUDE_ID,    1, ZSTR_ROLL,        UNIT_RADIANS,       3},
  {ATTITUDE_ID,    2, ZSTR_YAW,         UNIT_RADIANS,       3},
  {FLIGHT_MODE_ID, 0, ZSTR_FLIGHT_MODE, UNIT_TEXT,          0},
  {0,              0, "UNKNOWN",        UNIT_RAW,           0},
};

const CrossfireSensor & getCrossfireSensor(uint8_t id, uint8_t subId)
{
  if (id == LINK_ID)
    return crossfireSensors[RX_RSSI1_INDEX+subId];
  else if (id == BATTERY_ID)
    return crossfireSensors[BATT_VOLTAGE_INDEX+subId];
  else if (id == GPS_ID)
    return crossfireSensors[GPS_LATITUDE_INDEX+subId];
  else if (id == ATTITUDE_ID)
    return crossfireSensors[ATTITUDE_PITCH_INDEX+subId];
  else if (id == FLIGHT_MODE_ID)
    return crossfireSensors[FLIGHT_MODE_INDEX];
  else
    return crossfireSensors[UNKNOWN_INDEX];
}

void processCrossfireTelemetryValue(uint8_t index, uint32_t value)
{
  const CrossfireSensor & sensor = crossfireSensors[index];
  setTelemetryValue(TELEM_PROTO_CROSSFIRE, sensor.id, 0, sensor.subId, value, sensor.unit, sensor.precision);
}

bool checkCrossfireTelemetryFrameCRC()
{
  uint8_t len = telemetryRxBuffer[1];
  uint8_t crc = crc8(&telemetryRxBuffer[2], len-1);
  return (crc == telemetryRxBuffer[len+1]);
}

template<int N>
bool getCrossfireTelemetryValue(uint8_t index, uint32_t & value)
{
  bool result = false;
  value = 0;
  uint8_t * byte = &telemetryRxBuffer[index];
  for (uint8_t i=0; i<N; i++) {
    value <<= 8;
    if (*byte != 0xff) {
      result = true;
    }
    value += *byte++;
  }
  return result;
}

void processCrossfireTelemetryFrame()
{
  if (!checkCrossfireTelemetryFrameCRC()) {
    TRACE("processCrossfirePacket(): CRC error ");
    DUMP(telemetryRxBuffer, TELEMETRY_RX_PACKET_SIZE);
    return;
  }

  uint8_t id = telemetryRxBuffer[2];
  uint32_t value;
  switch(id) {
    case GPS_ID:
      if (getCrossfireTelemetryValue<4>(3, value))
        processCrossfireTelemetryValue(GPS_LATITUDE_INDEX, value/10);
      if (getCrossfireTelemetryValue<4>(7, value))
        processCrossfireTelemetryValue(GPS_LONGITUDE_INDEX, value/10);
      if (getCrossfireTelemetryValue<2>(11, value))
        processCrossfireTelemetryValue(GPS_GROUND_SPEED_INDEX, value);
      if (getCrossfireTelemetryValue<2>(13, value))
        processCrossfireTelemetryValue(GPS_HEADING_INDEX, value);
      if (getCrossfireTelemetryValue<2>(15, value))
        processCrossfireTelemetryValue(GPS_ALTITUDE_INDEX,  value - 1000);
      if (getCrossfireTelemetryValue<1>(17, value))
        processCrossfireTelemetryValue(GPS_SATELLITES_INDEX, value);
      break;

    case LINK_ID:
      telemetryStreaming = TELEMETRY_TIMEOUT10ms;
      for (int i=0; i<10; i++) {
        if (getCrossfireTelemetryValue<1>(3+i, value)) {
          processCrossfireTelemetryValue(i, value);
          if (i == 2) {
            telemetryData.rssi.set(value);
          }
        }
      }
      break;

    case BATTERY_ID:
      if (getCrossfireTelemetryValue<2>(3, value))
        processCrossfireTelemetryValue(BATT_VOLTAGE_INDEX, value);
      if (getCrossfireTelemetryValue<2>(5, value))
        processCrossfireTelemetryValue(BATT_CURRENT_INDEX, value);
      if (getCrossfireTelemetryValue<3>(7, value))
        processCrossfireTelemetryValue(BATT_CAPACITY_INDEX, value);
      break;

    case ATTITUDE_ID:
      if (getCrossfireTelemetryValue<2>(3, value))
        processCrossfireTelemetryValue(ATTITUDE_PITCH_INDEX, value/10);
      if (getCrossfireTelemetryValue<2>(5, value))
        processCrossfireTelemetryValue(ATTITUDE_ROLL_INDEX, value/10);
      if (getCrossfireTelemetryValue<2>(7, value))
        processCrossfireTelemetryValue(ATTITUDE_YAW_INDEX, value/10);
      break;

    case FLIGHT_MODE_ID:
    {
      const CrossfireSensor & sensor = crossfireSensors[FLIGHT_MODE_INDEX];
      for (int i=0; i<min<int>(16, telemetryRxBuffer[1]-2); i+=4) {
        uint32_t value = *((uint32_t *)&telemetryRxBuffer[3+i]);
        setTelemetryValue(TELEM_PROTO_CROSSFIRE, sensor.id, 0, sensor.subId, value, sensor.unit, i);
      }
      break;
    }
  }
}

void processCrossfireTelemetryData(uint8_t data)
{
  if (telemetryRxBufferCount == 0 && data != 0x00) {
    return;
  }

  if (telemetryRxBufferCount == 1 && (data < 2 || data > TELEMETRY_RX_PACKET_SIZE-2)) {
    telemetryRxBufferCount = 0;
    return;
  }

  if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
    telemetryRxBuffer[telemetryRxBufferCount++] = data;
  }
  else {
    TRACE("processCrossfirePacket(): length error ");
    DUMP(telemetryRxBuffer, TELEMETRY_RX_PACKET_SIZE);
    telemetryRxBufferCount = 0;
  }

  if (telemetryRxBufferCount > 4) {
    uint8_t length = telemetryRxBuffer[1];
    if (length + 2 == telemetryRxBufferCount) {
      processCrossfireTelemetryFrame();
      telemetryRxBufferCount = 0;
    }
  }
}

void crossfireSetDefault(int index, uint8_t id, uint8_t subId)
{
  TelemetrySensor & telemetrySensor = g_model.telemetrySensors[index];

  telemetrySensor.id = id;
  telemetrySensor.instance = subId;

  const CrossfireSensor & sensor = getCrossfireSensor(id, subId);
  TelemetryUnit unit = sensor.unit;
  if (unit == UNIT_GPS_LATITUDE || unit == UNIT_GPS_LONGITUDE)
    unit = UNIT_GPS;
  uint8_t prec = min<uint8_t>(2, sensor.precision);
  telemetrySensor.init(sensor.name, unit, prec);
  if (id == LINK_ID) {
    telemetrySensor.logs = true;
  }

  storageDirty(EE_MODEL);
}
