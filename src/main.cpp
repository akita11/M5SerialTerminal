#include <M5Unified.h>

// Based from serial_monitor.ino
// https://logikara.blog/serial_monitor/

// CardKBD: read I2C=0x5f

// M5Canvas
// https://github.com/m5stack/M5Unified/issues/7
M5Canvas canvas;

// 追加したい機能
// - 受信／送信の行末コード
// - 通信速度の変更
// - キーボード入力→送信

// PortC of Basic
#define PIN_RXD 16
#define PIN_TXD 17

char c;							 // 受信データ（文字）格納用
int c_cnt = -1;				// 文字数カウント
String row[15] = {};	// 行配列
bool disp_set = 0;		// 画面出力フラグ

/* --------------- 行スクロール処理関数 --------------- */
void scroll_disp() {
	for (int i = 1; i < 15; i++) {	// 最上行以外の14行分繰り返し
		row[i-1] = row[i];						// 行配列を左へシフト（上スクロール）
	}
	row[14] = "";									 // 最下行の表示を消去 
}
/* ---------- スクロール後の行一括表示処理関数 ---------- */
void disp_out() {								 // 表示実行（15行一括表示）
	canvas.fillScreen(BLACK);			 // 画面リセット（黒塗り潰し）
	canvas.setCursor(0, 0);				 // カーソル原点
	for (int i = 0; i < 15; i++) {	// 15行分繰り返し
		canvas.println(row[i]);			 // 行配列を改行しながら順番に表示
	}
	canvas.pushSprite(0, 0);				// メモリ内に描画したcanvasを座標を指定して表示する
}

#define RXbufSize 1024
char RXbuf[RXbufSize];
uint16_t pRXbuf_w = 0, pRXbuf_r = 0;

void setup() {
	auto cfg = M5.config();
	M5.begin(cfg);
	Serial.begin(9600);
	Serial1.begin(9600, SERIAL_8N1, PIN_RXD, PIN_TXD);
	
	// LCD初期設定
	canvas.setColorDepth(8);		// カラーモード設定
	canvas.createSprite(M5.Display.width(), M5.Display.height()); // canvasサイズ（メモリ描画領域）設定（画面サイズに設定）

	// スプライト表示設定（canvas.で指定してメモリ内の仮想画面に描画）
	canvas.fillScreen(BLACK);					 // 背景塗り潰し
	canvas.setTextColor(WHITE);				 // 文字色
	// 液晶初期表示（シリアルデータ通信速度、使用端子番号）
	canvas.setFont(&fonts::Font4);									// フォント設定
	canvas.println("Serial Monitor (9600bps)");		 // 初期表示
	canvas.setFont(&fonts::Font2);									// フォント設定
	canvas.println("Serial \n	RX = 3(R0)		/ TX = 1(T0)		/ GND = G");
	canvas.println("Serial1 (GROVE)\n	RX = 21				/ TX = 22			 / GND = G"); // BASIC, GRAYの場合
	// canvas.println("Serial1 (GROVE)\n	RX = 32				/ TX = 33			 / GND = G"); // ※CORE2の場合
	canvas.println("Serial2 \n	RX = 16(R2)	/ TX = 17(T2)	 / GND = G\n"); // BASIC, GRAYの場合
	// canvas.println("Serial2 \n	RX = 13(R2)	/ TX = 14(T2)	 / GND = G\n"); // ※CORE2の場合

	canvas.println("ButtonA : Scroll UP");
	canvas.println("ButtonB : Gothic");
	canvas.println("ButtonC : Mincho");
	canvas.pushSprite(0, 0);												// メモリ内に描画したcanvasを座標を指定して表示する

	canvas.setFont(&fonts::lgfxJapanGothic_16);		 // フォント日本語ゴシック（16px、等幅）
}
// メイン ------------------------------------------------------
void loop() {
	M5.update();	//本体のボタン状態更新
	// 受信データ処理
	while (Serial1.available()){
		RXbuf[pRXbuf_w] = Serial1.read();
		pRXbuf_w++; if (pRXbuf_w == RXbufSize) pRXbuf_w = 0;
	}
	while(pRXbuf_w != pRXbuf_r){
		disp_set = 1;																		 // 画面出力フラグ1
		c = RXbuf[pRXbuf_r];
		pRXbuf_r++; if (pRXbuf_r == RXbufSize) pRXbuf_r = 0;
		// 行スクロール処理（改行、半角、全角判定）
		if (c == '\n') { // 「改行」を受信したら
			c_cnt = 0;		 // 文字数カウントを0リセット
			scroll_disp(); // 行スクロール処理
		} else {				 // 「改行」でなければ（文字コードから全角半角を判定して文字数カウント）
			if (c <= 0x7F) {							// ASCII文字（半角）なら
				c_cnt++;										// 文字数カウント+1
			}
			if (c >= 0xE0 && c <= 0xEF) { // 全角なら（半角ｶﾅは全角としてカウント）
				c_cnt = c_cnt + 2;					// 文字数カウント+2
			}
			if (c_cnt >= 39) {	// 文字数カウントが39なら
				c_cnt = 0;				// 文字数カウント0リセット
				scroll_disp();		// 行スクロール処理
				disp_out();			 // スクロール後の行一括表示処理
			}
			row[14] += c;			 // 行配列の最下行に1文字づつ追加して文字列へ
		}
	}
	if (disp_set == 1) {	// 画面出力フラグが1なら
		disp_set = 0;			 // 画面出力フラグ0セット
		disp_out();				 // スクロール後の行一括表示処理
	}

	if (M5.BtnA.wasPressed()) { // ボタンAが押されていれば
		scroll_disp();						// 行スクロール処理
		disp_out();							 // スクロール後の行一括表示処理
	}
	if (M5.BtnB.wasPressed()) { // ボタンBが押されていれば
		canvas.setFont(&fonts::lgfxJapanGothic_16); // 日本語ゴシック体（16px、等幅）
		canvas.fillScreen(BLACK);									 // 背景塗り潰し
		canvas.setCursor(0, 0);										 // カーソルを(0, 0)へ
		canvas.println("フォント変更\nゴシック体");		 // フォント設定表示
		canvas.pushSprite(0, 0);										// メモリ内に描画したcanvasを座標を指定して表示する
	}
	if (M5.BtnC.wasPressed()) { // ボタンCが押されていれば
		canvas.setFont(&fonts::lgfxJapanMincho_16); // 日本語明朝体（16px、等幅）
		canvas.fillScreen(BLACK);									 // 背景塗り潰し
		canvas.setCursor(0, 0);										 // カーソルを(0, 0)へ
		canvas.println("フォント変更\n明朝体");				// フォント設定表示
		canvas.pushSprite(0, 0);										// メモリ内に描画したcanvasを座標を指定して表示する
	}
}
