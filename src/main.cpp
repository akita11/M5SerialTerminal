#include <M5Unified.h>

// Based from serial_monitor.ino
// https://logikara.blog/serial_monitor/

// ToDo:
// - Hex from CardKBD and send binary 

// PortA/C of Basic
uint8_t pinRXD[] = {22, 16};
uint8_t pinTXD[] = {21, 17};
uint16_t menuColor[] = {RED, CYAN};
uint8_t stPort = 0;

// Terminal to monitor
#define Terminal Serial1

uint8_t stBaud = 0;
#define N_BAUD 3
uint32_t baud[] = {9600, 57600, 115200};
uint8_t stTerminalCode = 0;
#define N_TERMINAL_CODE 3
String terminal_code[] = {"\r", "\n", "\r\n"};

uint8_t stBinary = 0;

// Basic's button position on display
// 34/[64]/30/[64]/30/[64]/34 = 320

void setBinaryMode(uint8_t stBinary)
{
	M5.Display.fillRect(34, 224, 64, 16, menuColor[stPort]);
	M5.Display.setCursor(40, 224);
	M5.Display.setTextColor(BLACK, menuColor[stPort]);
	if (stBinary == 0) M5.Display.printf("Text");
	else M5.Display.printf("Binary");
	M5.Display.setTextColor(WHITE, BLACK);
}

void setBaud(uint8_t stBaud)
{
	M5.Display.fillRect(128, 224, 64, 16, menuColor[stPort]);
	M5.Display.setTextColor(BLACK, menuColor[stPort]);
	M5.Display.setCursor(135, 224);
	M5.Display.printf("%d", baud[stBaud]);
	M5.Display.setTextColor(WHITE, BLACK);
	Terminal.end();
	Terminal.begin(baud[stBaud], SERIAL_8N1, pinRXD[stPort], pinTXD[stPort]);
	if (stPort == 1) 	M5.Ex_I2C.begin();
}

// CR=0x0d (\r)
// LF=0x0a (\n)
// stTerminalCode: 0=CR, 1=LF, 2=CRLF

void setTerminalCode(uint8_t stTerminalCode)
{
	M5.Display.fillRect(222, 224, 64, 16, menuColor[stPort]);
	M5.Display.setCursor(240, 224);
	M5.Display.setTextColor(BLACK, menuColor[stPort]);
	if (stTerminalCode == 0) M5.Display.printf("CR");
	else if (stTerminalCode == 1) M5.Display.printf("LF");
	else if (stTerminalCode == 2) M5.Display.printf("CRLF");
	M5.Display.setTextColor(WHITE, BLACK);
}

#define RXbufSize 1024
char RXbuf[RXbufSize];
uint16_t pRXbuf_w = 0, pRXbuf_r = 0;

// get char from CardKBD
uint8_t getKey(){
	uint8_t c;
	M5.Ex_I2C.start(0x5f, true, 100000);
	M5.Ex_I2C.read(&c, 1);
	M5.Ex_I2C.stop();
	return c;
}	

/*
	// setTextScroll 関数で、画面下端に到達した時のスクロール動作を指定します。
  // setScrollRect 関数でスクロールする矩形範囲を指定します。(未指定時は画面全体がスクロールします)
  // ※ スクロール機能は、LCDが画素読出しに対応している必要があります。
  lcd.setTextScroll(true);

  // 第１～第４引数で X Y Width Height の矩形範囲を指定し、第５引数でスクロール後の色を指定します。第５引数は省略可(省略時は変更なし)
  lcd.setScrollRect(10, 10, lcd.width() - 20, lcd.height() - 20, 0x00001FU);
*/

void setup() {
	auto cfg = M5.config();
	M5.begin(cfg);

	M5.Display.fillScreen(BLACK);
	M5.Display.setTextColor(WHITE);
	M5.Display.setFont(&fonts::lgfxJapanGothic_16);
	M5.Display.setScrollRect(0, 0, 320, 224);
	M5.Display.setTextScroll(true);
	M5.Display.printf("M5 Serial Monitor\n");
	setBinaryMode(stBinary);
	setBaud(stBaud);
	setTerminalCode(stTerminalCode);
}

uint16_t c_cnt = 0;

void loop() {
	char c;
	M5.update();
	if (stPort == 1){
		c = getKey();
		if (c != 0){
//			printf("%02x\n", c);
			if (c == 0x0d){
				if (stTerminalCode == 0) Terminal.write(0x0d);
				else if (stTerminalCode == 1) Terminal.write(0x0a);
				else if (stTerminalCode == 2) {Terminal.write(0x0d); Terminal.write(0x0a);}
			}
			else Terminal.write(c);
		}
	} 

	while (Terminal.available()){
		RXbuf[pRXbuf_w] = Terminal.read();
		pRXbuf_w++; if (pRXbuf_w == RXbufSize) pRXbuf_w = 0;
	}

	while(pRXbuf_w != pRXbuf_r){
		c = RXbuf[pRXbuf_r];
		pRXbuf_r++; if (pRXbuf_r == RXbufSize) pRXbuf_r = 0;
		uint8_t fTerm = 0;
		if (((stTerminalCode == 0 || stTerminalCode == 2) && c == '\r')
		|| ((stTerminalCode == 1)  && c == '\n')) {
			fTerm = 1;
		}
		if (fTerm == 1) M5.Display.printf("\n");
		else {
			if (stBinary == 0){
				M5.Display.printf("%c", c);
				if (c <= 0x7f) { // for ASCII
					c_cnt++;
				}
				if (c >= 0xe0 && c <= 0xef) {
					c_cnt = c_cnt + 2;
				}
				if (c_cnt >= 39) {
					c_cnt = 0;
					M5.Display.printf("\n");
				}
			}
			else{
				M5.Display.printf("%02x ", c);
				c_cnt += 3;
				if (c_cnt >= 38) {
					c_cnt = 0;
					M5.Display.printf("\n");
				}
			}
		}
	}

	if (M5.BtnA.wasPressed()) {
		stBinary = 1 - stBinary;
		setBinaryMode(stBinary);
	}
	if (M5.BtnB.wasPressed()) {
		uint8_t tm = 0;
		while(M5.BtnB.isPressed() && tm < 20) {
			M5.update();
			tm++;
			delay(100);
		}
		if (tm < 20){
			// short B ppress
			stBaud = (stBaud + 1) % N_BAUD;
		}
		else{
			// long B ppress
			stPort = (stPort + 1) % 2;
			setBinaryMode(stBinary);
			setTerminalCode(stTerminalCode);
		}
		setBaud(stBaud);
	}
	if (M5.BtnC.wasPressed()) {
		stTerminalCode = (stTerminalCode + 1) % N_TERMINAL_CODE;
		setTerminalCode(stTerminalCode);
	}
}
