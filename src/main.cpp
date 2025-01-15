#include <M5Unified.h>

// Based from serial_monitor.ino
// https://logikara.blog/serial_monitor/

// M5Canvas
// https://github.com/m5stack/M5Unified/issues/7
M5Canvas canvas(&M5.Lcd);

// 追加したい機能
// - キーボード入力→送信

// PortC of Basic
#define PIN_RXD 16
#define PIN_TXD 17

// Terminal to monitor
#define Terminal Serial

#define N_LINE 14
char c;
int c_cnt = -1;
String row[N_LINE] = {};
bool disp_set = 0;

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
	canvas.fillRect(34, 224, 64, 16, WHITE);
	canvas.setCursor(40, 224);
	canvas.setTextColor(BLACK);
	if (stBinary == 0) canvas.printf("Text");
	else canvas.printf("Binary");
	canvas.setTextColor(WHITE);
	canvas.pushSprite(0, 0);
}

void setBaud(uint8_t stBaud)
{
	canvas.fillRect(128, 224, 64, 16, WHITE);
	canvas.setTextColor(BLACK);
	canvas.setCursor(135, 224); canvas.printf("%d", baud[stBaud]);
	canvas.setTextColor(WHITE);
	canvas.pushSprite(0, 0);
	Terminal.end();
	Terminal.begin(baud[stBaud], SERIAL_8N1, PIN_RXD, PIN_TXD);
}

void setTerminalCode(uint8_t stTerminalCode)
{
	canvas.fillRect(222, 224, 64, 16, WHITE);
	canvas.setCursor(240, 224);
	canvas.setTextColor(BLACK);
	if (stTerminalCode == 0) canvas.printf("CR");
	else if (stTerminalCode == 1) canvas.printf("LF");
	else if (stTerminalCode == 2) canvas.printf("CRLF");
	canvas.setTextColor(WHITE);
	canvas.pushSprite(0, 0);
}

void scroll_disp() {
	for (int i = 1; i < N_LINE; i++) {
		row[i-1] = row[i];
	}
	row[N_LINE - 1] = "";
}

void disp_out() {
	canvas.fillRect(0, 0, 320, 224, BLACK);
	canvas.setCursor(0, 0);
	for (int i = 0; i < N_LINE; i++) {
		canvas.println(row[i]);
	}
	canvas.pushSprite(0, 0);
}

#define RXbufSize 1024
char RXbuf[RXbufSize];
uint16_t pRXbuf_w = 0, pRXbuf_r = 0;

// get char from CardKBD
char getKey(){
	char c;
	c = M5.Ex_I2C.readRegister8(0x5f, 0x00, 400000);
	return c;
}	


void setup() {
	auto cfg = M5.config();
	M5.begin(cfg);
	
  M5.Ex_I2C.begin();

	canvas.setColorDepth(8);
	canvas.createSprite(M5.Display.width(), M5.Display.height());

	canvas.fillScreen(BLACK);
	canvas.setTextColor(WHITE);
	canvas.setFont(&fonts::lgfxJapanGothic_16);
	canvas.printf("Serial Monitor\n");
	setBinaryMode(stBinary);
	setBaud(stBaud);
	setTerminalCode(stTerminalCode);
	canvas.pushSprite(0, 0);
}

void loop() {
	M5.update();
	while (Terminal.available()){
		RXbuf[pRXbuf_w] = Serial.read();
		pRXbuf_w++; if (pRXbuf_w == RXbufSize) pRXbuf_w = 0;
	}
	while(pRXbuf_w != pRXbuf_r){
		disp_set = 1;
		c = RXbuf[pRXbuf_r];
		pRXbuf_r++; if (pRXbuf_r == RXbufSize) pRXbuf_r = 0;
		if (c == '\n') {
			c_cnt = 0;
			scroll_disp();
		} 
		else {
			if (stBinary == 0){
				if (c <= 0x7f) { // for ASCII
					c_cnt++;
				}
				if (c >= 0xe0 && c <= 0xef) {
					c_cnt = c_cnt + 2;
				}
				if (c_cnt >= 39) {
					c_cnt = 0;
					scroll_disp();
					disp_out();
				}
				row[N_LINE - 1] += c;
			}
			else{
				if (c < 0x10) row[N_LINE - 1] += "0";
				row[N_LINE - 1] +=String(c, HEX);
				row[N_LINE - 1] += " ";
				c_cnt += 3;
				if (c_cnt >= 38) {
					c_cnt = 0;
					scroll_disp();
					disp_out();
				}
			}
		}
	}
	if (disp_set == 1) {
		disp_set = 0;
		disp_out();
	}

	if (M5.BtnA.wasPressed()) {
		stBinary = 1 - stBinary;
		setBinaryMode(stBinary);
	}
	if (M5.BtnB.wasPressed()) {
		stBaud = (stBaud + 1) % N_BAUD;
		setBaud(stBaud);
	}
	if (M5.BtnC.wasPressed()) {
		stTerminalCode = (stTerminalCode + 1) % N_TERMINAL_CODE;
		setTerminalCode(stTerminalCode);
	}
}
