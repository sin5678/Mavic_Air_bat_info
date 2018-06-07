// thanks to https://github.com/PowerCartel/PackProbe

#include <Wire.h>
byte deviceAddress = 11;

// Standard and common non-standard Smart Battery commands
#define BATTERY_MODE             0x03
#define TEMPERATURE              0x08
#define VOLTAGE                  0x09
#define CURRENT                  0x0A
#define RELATIVE_SOC             0x0D
#define ABSOLUTE_SOC             0x0E
#define REMAINING_CAPACITY       0x0F
#define FULL_CHARGE_CAPACITY     0x10
#define TIME_TO_FULL             0x13
#define CHARGING_CURRENT         0x14
#define CHARGING_VOLTAGE         0x15
#define BATTERY_STATUS           0x16
#define CYCLE_COUNT              0x17
#define DESIGN_CAPACITY          0x18
#define DESIGN_VOLTAGE           0x19
#define SPEC_INFO                0x1A
#define MFG_DATE                 0x1B
#define SERIAL_NUM               0x1C
#define MFG_NAME                 0x20   // String
#define DEV_NAME                 0x21   // String
#define CELL_CHEM                0x22   // String
#define MFG_DATA                 0x23   // String
#define CELL4_VOLTAGE            0x3C   // Indidual cell voltages don't work on Lenovo and Dell Packs
#define CELL3_VOLTAGE            0x3D
#define CELL2_VOLTAGE            0x3E
#define CELL1_VOLTAGE            0x3F
#define STATE_OF_HEALTH          0x4F

#define bufferLen 32
uint8_t i2cBuffer[bufferLen];

void setup()
{
  Serial.begin(115200);  // start serial for output
  
  while (!Serial) {
    ; // wait for Console port to connect.
  }

  Serial.println("Console Initialized");

  Wire.begin();
  Wire.setClock(100000);

}

uint8_t read_byte()
{
  while (1)
  {
    if (Wire.available())
    {
      return Wire.read();
    }
  }
}

int fetchWord(byte func)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(func);
  Wire.endTransmission(false);
  delay(1);// FIX wire bug
  Wire.requestFrom(deviceAddress, 2, true);
  
  uint8_t b1 = read_byte();
  uint8_t b2 = read_byte();
  Wire.endTransmission();
  return (int)b1 | ((( int)b2) << 8);

  /*
    i2c_start(deviceAddress << 1 | I2C_WRITE);
    i2c_write(func);
    i2c_rep_start(deviceAddress << 1 | I2C_READ);
  */
}



uint8_t i2c_smbus_read_block ( uint8_t command, uint8_t* blockBuffer, uint8_t blockBufferLen )
{
  uint8_t x, num_bytes;
  Wire.beginTransmission(deviceAddress);
  Wire.write(command);
  Wire.endTransmission(false);
  delay(1);
  Wire.requestFrom(deviceAddress, blockBufferLen, true);
  
  num_bytes = read_byte();
  num_bytes = constrain(num_bytes, 0, blockBufferLen - 2);
  for (x = 0; x < num_bytes - 1; x++) { // -1 because x=num_bytes-1 if x<y; last byte needs to be "nack"'d, x<y-1
    blockBuffer[x] = read_byte();
  }
  blockBuffer[x++] = read_byte(); // this will nack the last byte and store it in x's num_bytes-1 address.
  blockBuffer[x] = 0; // and null it at last_byte+1
  Wire.endTransmission();
  return num_bytes;

  /*
    i2c_start((deviceAddress << 1) + I2C_WRITE);
    i2c_write(command);
    i2c_rep_start((deviceAddress << 1) + I2C_READ);
    num_bytes = i2c_read(false); // num of bytes; 1 byte will be index 0
    num_bytes = constrain(num_bytes, 0, blockBufferLen - 2); // room for null at the end
    for (x = 0; x < num_bytes - 1; x++) { // -1 because x=num_bytes-1 if x<y; last byte needs to be "nack"'d, x<y-1
    blockBuffer[x] = i2c_read(false);
    }
    blockBuffer[x++] = i2c_read(true); // this will nack the last byte and store it in x's num_bytes-1 address.
    blockBuffer[x] = 0; // and null it at last_byte+1
    i2c_stop();
    return num_bytes;
  */
}

void loop()
{
  uint8_t length_read = 0;

  Serial.print("Manufacturer Name: ");
  length_read = i2c_smbus_read_block(MFG_NAME, i2cBuffer, bufferLen);
  Serial.write(i2cBuffer, length_read);
  Serial.println("");

  Serial.print("Device Name: ");
  length_read = i2c_smbus_read_block(DEV_NAME, i2cBuffer, bufferLen);
  Serial.write(i2cBuffer, length_read);
  Serial.println("");

  Serial.print("Chemistry ");
  length_read = i2c_smbus_read_block(CELL_CHEM, i2cBuffer, bufferLen);
  Serial.write(i2cBuffer, length_read);
  Serial.println("");

  // Console.print("Manufacturer Data ");
  // length_read = i2c_smbus_read_block(MFG_DATA, i2cBuffer, bufferLen);
  // Console.write(i2cBuffer, length_read);
  // Console.println("");

  Serial.print("Design Capacity: " );
  Serial.println(fetchWord(DESIGN_CAPACITY));

  Serial.print("Design Voltage: " );
  Serial.println(fetchWord(DESIGN_VOLTAGE));

  String formatted_date = "Manufacture Date (Y-M-D): ";
  int mdate = fetchWord(MFG_DATE);
  int mday = B00011111 & mdate;
  int mmonth = mdate >> 5 & B00001111;
  int myear = 1980 + (mdate >> 9 & B01111111);
  formatted_date += myear;
  formatted_date += "-";
  formatted_date += mmonth;
  formatted_date += "-";
  formatted_date += mday;
  Serial.println(formatted_date);

  Serial.print("Serial Number: ");
  Serial.println(fetchWord(SERIAL_NUM));

  Serial.print("Specification Info: ");
  Serial.println(fetchWord(SPEC_INFO));

  Serial.print("Cycle Count: " );
  Serial.println(fetchWord(CYCLE_COUNT));

  Serial.print("Voltage: ");
  Serial.println((float)fetchWord(VOLTAGE) / 1000);

  Serial.print("Full Charge Capacity: " );
  Serial.println(fetchWord(FULL_CHARGE_CAPACITY));

  Serial.print("Remaining Capacity: " );
  Serial.println(fetchWord(REMAINING_CAPACITY));

  Serial.print("Relative Charge(%): ");
  Serial.println(fetchWord(RELATIVE_SOC));

  Serial.print("Absolute Charge(%): ");
  Serial.println(fetchWord(ABSOLUTE_SOC));

  Serial.print("Minutes remaining for full charge: ");
  Serial.println(fetchWord(TIME_TO_FULL));

  // These aren't part of the standard, but work with some packs.
  // They don't work with the Lenovo and Dell packs we've tested
  Serial.print("Cell 1 Voltage: ");
  Serial.println(fetchWord(CELL1_VOLTAGE));
  Serial.print("Cell 2 Voltage: ");
  Serial.println(fetchWord(CELL2_VOLTAGE));
  Serial.print("Cell 3 Voltage: ");
  Serial.println(fetchWord(CELL3_VOLTAGE));
  Serial.print("Cell 4 Voltage: ");
  Serial.println(fetchWord(CELL4_VOLTAGE));

  Serial.print("State of Health: ");
  Serial.println(fetchWord(STATE_OF_HEALTH));

  Serial.print("Battery Mode (BIN): 0b");
  Serial.println(fetchWord(BATTERY_MODE), BIN);

  Serial.print("Battery Status (BIN): 0b");
  Serial.println(fetchWord(BATTERY_STATUS), BIN);

  Serial.print("Charging Current: ");
  Serial.println(fetchWord(CHARGING_CURRENT));

  Serial.print("Charging Voltage: ");
  Serial.println(fetchWord(CHARGING_VOLTAGE));

  Serial.print("Temp: ");
  unsigned int tempk = fetchWord(TEMPERATURE);
  Serial.println((float)tempk / 10.0 - 273.15);

  Serial.print("Current (mA): " );
  Serial.println(fetchWord(CURRENT));

  Serial.println(".");
  delay(5000);
}
