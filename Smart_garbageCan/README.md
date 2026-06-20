# Thùng Rác Thông Minh — Smart Can

Hệ thống tự động mở nắp thùng rác khi phát hiện vật thể tiếp cận, sử dụng cảm biến siêu âm HC-SR04 để đo khoảng cách và servo SG90 để điều khiển nắp. Xây dựng trên nền tảng Arduino UNO với PlatformIO.

---

## Mục lục

1. [Tính năng](#1-tính-năng)
2. [Phần cứng và sơ đồ chân](#2-phần-cứng-và-sơ-đồ-chân)
3. [Nguyên lý hoạt động](#3-nguyên-lý-hoạt-động)
4. [Mô tả code](#4-mô-tả-code)
5. [Thông số cấu hình](#5-thông-số-cấu-hình)
6. [Cài đặt và nạp code](#6-cài-đặt-và-nạp-code)
7. [Giám sát qua Serial Monitor](#7-giám-sát-qua-serial-monitor)
8. [Hạn chế và hướng phát triển](#8-hạn-chế-và-hướng-phát-triển)

---

## 1. Tính năng

- Tự động mở nắp khi phát hiện vật thể trong vòng 20 cm.
- Giữ nắp mở trong 3.5 giây để người dùng bỏ rác, sau đó tự động đóng lại.
- Lọc nhiễu bằng cách lấy trung bình 3 lần đo liên tiếp trước mỗi quyết định.
- Tắt tín hiệu điều khiển servo sau khi đóng nắp (`servo.detach()`) để tránh servo bị rung và tiêu hao điện không cần thiết khi đứng yên.
- In khoảng cách đo được ra Serial Monitor để dễ dàng debug và hiệu chỉnh ngưỡng.

---

## 2. Phần cứng và sơ đồ chân

### Linh kiện

| Linh kiện | Số lượng | Chức năng |
|---|---|---|
| Arduino UNO (ATmega328P) | 1 | Vi điều khiển chính |
| Servo SG90 | 1 | Điều khiển nắp thùng rác |
| Cảm biến siêu âm HC-SR04 | 1 | Đo khoảng cách phát hiện người |
| Dây nối | — | Kết nối các linh kiện |
| Nguồn 5V (USB hoặc adapter) | 1 | Cấp nguồn toàn bộ hệ thống |

### Sơ đồ chân

| Chân Arduino | Kết nối | Ghi chú |
|---|---|---|
| Pin 5 | HC-SR04 TRIG | OUTPUT — gửi xung kích phát |
| Pin 6 | HC-SR04 ECHO | INPUT — nhận xung phản hồi |
| Pin 9 | Servo SG90 (Signal) | PWM OUTPUT |
| 5V | HC-SR04 VCC + Servo VCC | Cấp nguồn 5V |
| GND | HC-SR04 GND + Servo GND | Nối đất chung |

### Sơ đồ kết nối

```
Arduino UNO
┌────────────────────┐
│                    │      ┌─────────────┐
│  Pin 5 (TRIG) ─────┼─────►│ HC-SR04     │
│  Pin 6 (ECHO) ◄────┼──────┤ TRIG / ECHO │
│  5V           ─────┼─────►│ VCC         │
│  GND          ─────┼─────►│ GND         │
│                    │      └─────────────┘
│                    │
│  Pin 9 (PWM)  ─────┼──────► Servo SG90 (Signal - dây cam)
│  5V           ─────┼──────► Servo SG90 (VCC    - dây đỏ)
│  GND          ─────┼──────► Servo SG90 (GND    - dây nâu/đen)
└────────────────────┘
```

---

## 3. Nguyên lý hoạt động

### Đo khoảng cách bằng HC-SR04

HC-SR04 hoạt động bằng cách phát một xung siêu âm 40 kHz qua chân TRIG (kéo HIGH 10 µs), rồi đo thời gian xung dội lại qua chân ECHO. Khoảng cách được tính theo công thức:

```
Khoảng cách (cm) = thời gian ECHO (µs) / 58
```

Lý do chia 58: tốc độ âm thanh ≈ 340 m/s = 0.034 cm/µs. Âm thanh đi và về hai chiều nên chia đôi → 0.034 / 2 = 0.017 cm/µs, đảo lại thành 1 / 0.017 ≈ 58.

### Lọc nhiễu bằng trung bình 3 mẫu

Mỗi chu kỳ loop, hệ thống đo 3 lần liên tiếp cách nhau 10 ms rồi lấy giá trị trung bình. Kỹ thuật này loại bỏ các giá trị đột biến do nhiễu điện hoặc phản xạ sai từ bề mặt nghiêng.

```
khcach_tb = (averDist[0] + averDist[1] + averDist[2]) / 3
```

### Điều khiển nắp

```
Khoảng cách ≤ 20 cm  →  Mở nắp (0°) → Giữ 3.5 giây → [tiếp tục đo]
Khoảng cách > 20 cm  →  Đóng nắp (90°) → Ngắt tín hiệu servo
```

### Tại sao dùng `servo.detach()`

Sau khi đóng nắp, `servo.detach()` ngắt tín hiệu PWM khỏi chân servo. Điều này giúp:
- Loại bỏ tiếng rung/vo ve của servo khi đứng yên nhưng vẫn nhận xung PWM.
- Giảm tiêu thụ điện khi không cần giữ vị trí (nắp được giữ bằng cơ học hoặc trọng lực).
- Tránh nhiễu cho các chân PWM khác trên board.

---

## 4. Mô tả code

### Hàm `setup()`

```cpp
void setup() {
    Serial.begin(9600);       // Khởi động Serial để debug
    pinMode(triPin, OUTPUT);  // TRIG là output
    pinMode(EchPin, INPUT);   // ECHO là input

    servo.attach(servoPin);   // Gắn servo vào pin 9
    servo.write(closeAngle);  // Đặt về vị trí đóng (90°) lúc khởi động
    delay(100);               // Chờ servo chạy đến vị trí
    servo.detach();           // Ngắt PWM, servo giữ nguyên vị trí đóng
}
```

Khi khởi động, hệ thống luôn đưa nắp về trạng thái đóng trước, đảm bảo trạng thái ban đầu xác định dù servo có bị lệch trước đó.

### Hàm `loop()`

```cpp
void loop() {
    // Bước 1: Đo 3 lần
    for (int i = 0; i <= 2; i++) {
        khcach = readDistance();
        averDist[i] = khcach;
        delay(10);
    }

    // Bước 2: Tính trung bình
    khcach_tb = (averDist[0] + averDist[1] + averDist[2]) / 3;
    Serial.println(khcach_tb);  // In ra Serial Monitor

    // Bước 3: Quyết định
    if (khcach_tb <= nguong_khcach) {
        servo.attach(servoPin);
        delay(1);               // Chờ PWM ổn định
        servo.write(openAngle); // Mở nắp (0°)
        delay(3500);            // Giữ mở 3.5 giây
    } else {
        servo.write(closeAngle); // Đóng nắp (90°)
        delay(1000);             // Chờ servo đóng xong
        servo.detach();          // Ngắt tín hiệu
    }
}
```

### Hàm `readDistance()`

```cpp
float readDistance() {
    digitalWrite(triPin, LOW);
    delayMicroseconds(2);        // Đảm bảo TRIG bắt đầu từ LOW

    digitalWrite(triPin, HIGH);
    delayMicroseconds(10);       // Xung kích 10 µs
    digitalWrite(triPin, LOW);

    float khcach = pulseIn(EchPin, HIGH) / 58.00;  // Tính khoảng cách
    return khcach;
}
```

`pulseIn(EchPin, HIGH)` đo thời gian chân ECHO ở mức HIGH tính bằng micro giây. Chia cho 58 để ra đơn vị cm.

---

## 5. Thông số cấu hình

Tất cả thông số có thể điều chỉnh đều được khai báo dưới dạng hằng số ở đầu file:

| Hằng số | Giá trị mặc định | Ý nghĩa |
|---|---|---|
| `servoPin` | 9 | Chân PWM kết nối servo |
| `openAngle` | 0° | Góc mở nắp hoàn toàn |
| `closeAngle` | 90° | Góc đóng nắp |
| `triPin` | 5 | Chân TRIG của HC-SR04 |
| `EchPin` | 6 | Chân ECHO của HC-SR04 |
| `nguong_khcach` | 20 cm | Ngưỡng khoảng cách kích hoạt mở nắp |
| `delay(3500)` trong loop | 3500 ms | Thời gian giữ nắp mở |

Để thay đổi khoảng cách kích hoạt, sửa dòng:
```cpp
const int nguong_khcach = 20; // đổi thành giá trị mong muốn (cm)
```

Để thay đổi thời gian giữ nắp mở, sửa dòng:
```cpp
delay(3500); // đổi thành số ms mong muốn
```

---

## 6. Cài đặt và nạp code

### Yêu cầu

- [Visual Studio Code](https://code.visualstudio.com/)
- Extension [PlatformIO IDE](https://platformio.org/platformio-ide) cho VS Code
- Driver USB cho Arduino UNO (CH340 hoặc ATmega16U2 tùy board)

### Cấu hình PlatformIO (`platformio.ini`)

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
lib_deps = arduino-libraries/Servo@^1.3.0
```

Thư viện `Servo` sẽ được PlatformIO tự động tải về khi build lần đầu.

### Các bước

1. Cài VS Code và extension PlatformIO IDE.
2. Mở VS Code, chọn **PlatformIO Home → Open Project**, trỏ đến thư mục `SMART_CAN`.
3. Cắm Arduino UNO vào máy tính qua USB, kiểm tra cổng COM trong Device Manager.
4. Nhấn **Build** (biểu tượng dấu tích ✓ ở thanh dưới) để biên dịch.
5. Nhấn **Upload** (biểu tượng mũi tên →) để nạp firmware.
6. Mở **Serial Monitor** (biểu tượng phích cắm) với baud rate 9600 để xem khoảng cách đo được.

---

## 7. Giám sát qua Serial Monitor

Sau khi nạp code, mở Serial Monitor (baud 9600). Mỗi chu kỳ loop, hệ thống in ra khoảng cách trung bình (cm):

```
45
43
44
18       ← vật thể đến gần, nắp mở
17
16
15
44       ← rút tay ra, nắp đóng
45
```

Dùng giá trị này để hiệu chỉnh `nguong_khcach` cho phù hợp với thực tế lắp đặt.

---

## 8. Hạn chế và hướng phát triển

### Hạn chế hiện tại

**Blocking delay:** Toàn bộ vòng lặp dùng `delay()`, nghĩa là CPU bị khóa trong suốt 3.5 giây giữ nắp mở — không thể xử lý thêm bất kỳ sự kiện nào trong lúc đó (ví dụ phát hiện tay rút ra sớm hơn để đóng nắp ngay).

**Không có phản hồi vị trí servo:** Hệ thống không biết servo đã thực sự đến đúng góc hay chưa — đây là điều khiển vòng hở (open-loop). Nếu servo bị kẹt cơ học, code vẫn tiếp tục bình thường.

**Nhiễu HC-SR04:** Cảm biến siêu âm có thể cho kết quả sai khi gặp bề mặt nghiêng, vật liệu mềm hút âm (vải, xốp), hoặc nhiều vật thể cùng lúc. Trung bình 3 mẫu giảm nhưng không loại hẳn nhiễu này.

**Không có chỉ báo trạng thái:** Người dùng không biết hệ thống đang ở trạng thái nào (đang mở, đang đóng, đang đo) nếu không nhìn trực tiếp vào nắp.

### Hướng phát triển

- Dùng `millis()` thay `delay()` để vòng lặp không bị block, cho phép đóng nắp ngay khi tay rút ra trước 3.5 giây.
- Thêm **LED hoặc buzzer** báo hiệu trạng thái mở/đóng nắp.
- Thêm **cảm biến hồng ngoại (IR)** bên trong thùng để phát hiện mức rác đầy và cảnh báo.
- Thêm **màn hình LCD** hoặc **OLED** hiển thị khoảng cách và trạng thái nắp.
- Nâng cấp lên **ESP32** để thêm kết nối WiFi, gửi thông báo qua điện thoại khi thùng đầy.
