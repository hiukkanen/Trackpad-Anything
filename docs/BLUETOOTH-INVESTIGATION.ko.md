# Bluetooth 수신 조사 — 2026-11-14

현재 상태: **Bluetooth 제스처 미완성**. USB 앱, USBHelper, 기존 GestureEngine은 이번 조사에서 수정하지 않았다.

## 재사용할 구현

- Linux `drivers/hid/hid-magicmouse.c`: Bluetooth feature `F1 02 01`, input `31`, 4바이트 헤더와 9바이트 접점. 입력 보고서 31을 드라이버에서 별도 등록한다.
  https://github.com/torvalds/linux/blob/master/drivers/hid/hid-magicmouse.c
- Windows `AmtPtpHidFilter/Device.c`, `Input.c`: 같은 초기화 명령을 사용하고 HIDCLASS 아래 transport에 `IOCTL_HID_READ_REPORT`를 보낸다. macOS의 일반 입력 콜백과 같은 경로가 아니다.
  https://github.com/vitoplantamura/MagicTrackpad2ForWindows
- `Sources/Bluetooth/Packet.h`: Bluetooth 접점 프레임을 기존 USB decoder 형식으로 변환.
- `Sources/Bluetooth/ReportStream.h`: HIDP DATA(input) 포장, 단일 31, 두 보고서를 묶은 F7 처리. 전체 유효성 확인 후 callback을 호출한다. ACL 조각 재조립은 포함하지 않으며 완성된 SDU가 필요하다. 실기기의 F7 수신은 아직 확인하지 못했다.

## 실제 확인한 사실

1. Sonoma HID descriptor/input elements는 02, 55, 90을 포함하지만 31은 없다. 일반 mouse 02 보고서는 모니터에 들어온다.
2. feature F1 02 01 요청의 반환값은 성공했지만, 그 뒤 터치 보고서는 콜백에 들어오지 않았다. 사용자 전원 재시작으로 일반 포인터가 복구됐다. 수신 경로 없이 이 명령을 반복하지 않는다.
3. IOBluetooth의 PSM19 async 호출이 0을 반환한 것은 요청 접수뿐이었다. bluetoothd 로그는 연결 실패454를 기록했고 CID는0이었다.
4. 연결을 끊고 PSM17부터 다시 열어도 실패했다. 기본 연결은 자동 복구했다.
5. 공개 IOBluetoothIgnoreHIDDevice API를 해당 기기에 호출한 테스트에서도 기본 HID 경로가 재연결됐다. 실험 종료 시 RemoveIgnoredHIDDevice를 호출했다. API가 이 환경에서 배타적 소유를 보장한다고 가정하지 않는다.
6. 기존 kernel L2CAP channel에 IOServiceOpen(type0)은 unsupported(e00002c7) 반환. public withObjectID로 registry object를 찾는 시도도 nil이었다.
7. 일반 블루투스 기기 캐시는 관리자 읽기 권한이 필요하다. read-cache-metadata.py는 이름에 trackpad가 들어간 레코드의 필드명/타입/길이만 출력한다. pairing 정보와 원본값을 출력하거나 수정하지 않는다.

## 검증과 다음 단계

`test-packet.c`, `test-stream.c` 통과. stream 테스트는 AddressSanitizer/UndefinedBehaviorSanitizer로 단일 접점,15접점,묶음 보고서,불완전 보고서,두 번째 보고서 오류 시 부분 전달 방지를 확인했다. 실제 Bluetooth 제스처가 된다는 증거는 아니다.

관리자 인증 후 캐시 메타데이터를 확인해 보고서 등록 경로를 평가한다. 기존 USB 설치본 교체, 시스템 확장 설치, SIP 변경, pairing cache 변경은 수행하지 않았다. 수신이 입증되기 전에는 GUI에서 Bluetooth 제스처 지원 완료로 표시하지 않는다.

## 후속 캐시 실험 결과

관리자 읽기 진단은 완료됐다. 해당 기기의 `HIDDescriptor`는135바이트이고 descriptor size 필드가 존재했다. 이후 **별도의 30초 제한 실험**에서 대상 모델804의 descriptor와 size 두 필드만 일시 변경했다. 기존 pairing 정보는 출력/별도 저장하지 않았다. 동일 Mac 내장 트랙패드의 가변 길이 vendor collection(FF00/25)을 참조해 input31을 추가했다. 원본 캐시 전체를 덮어 복구하는 방식 대신, 현재 캐시에서 변경한 두 필드만 되돌리도록 구현했다.

결과: `TRIAL_READY descriptorLength=158`, 재연결 후 실제 descriptor는135/IDs2,85,144, 종료 시 `ORIGINAL_ALREADY_PRESENT`. macOS가 원본 정보를 다시 저장했다. 활성 수신 경로도 바뀌지 않았다. 이 결과를 보고 bluetoothd 전체 재시작, 시스템 드라이버 수정, 보안 설정 변경으로 확대하지 않았다.

다음 후보는 HIDDriverKit 수신 드라이버이다. 아직 구현·로드·실기기 검증하지 않았다. Xcode16.2의 DriverKit24.2 SDK는 존재하지만 기본 xcrun은 CommandLineTools를 가리키므로 DEVELOPER_DIR 지정이 필요하다. 현재 확인된 코드 서명 identity는 로컬 개발 인증서3개뿐이다. 정상적인 DriverKit 활성화에는 적절한 Apple entitlement/profile이 필요하다. 이것이 모든 대안이 불가능하다는 증거는 아니다.
https://developer.apple.com/documentation/driverkit/requesting-entitlements-for-driverkit-development

## GET_REPORT 폴링 실험 (2026-11-14)

`experiments/bluetooth/getreport-probe.m`. 인터럽트 채널 대신 control 채널 GET_REPORT(Input, 0x31) 응답이 사용자 공간에 도달하는지 시험.

- A(읽기 전용): input02, featureF1 GET_REPORT 성공(약 20–30ms 왕복). input31은 200회 모두 `e00002c6`(kIOReturnBadMessageID).
- B(F1 02 01 후): 설정 성공(0x0), input31은 200회 모두 동일 오류. 로그 `experiments/bluetooth/getreport-probe-enable.log`.

결론: 이 경로로는 0x31을 얻을 수 없다. 사용자 공간 단독 경로 후보는 소진됐다. 남은 경로는 커널 측 수신(DriverKit 유료 개발 서명, 또는 보안 수준 하향을 동반한 kext/dext)이다.

## PID 위장 성공 (2026-11-14)

출처: GianMarioBen/MagicTrackPadCatalina PR #1 (Catalina, SIP 유지). Sonoma 이식 `experiments/bluetooth/pid-spoof.py`.

- 캐시: `/Library/Bluetooth/Library/Preferences/com.apple.MobileBluetooth.devices.plist`, 대상 레코드의 `HIDProductID`(LE16)와 `SdpCache` 내부 Device ID 속성만 0x0324→0x0265. SdpCache는 표준 SDP가 아닌 `attrID LE16 | type | size LE16 | value LE` 형식이며 VendorID(0x0201=0x004C) 패턴으로 위치를 교차 확인한다.
- 쓰기 직후 `killall -9 bluetoothd cfprefsd`. 이전 descriptor 실험과 달리 재시작 후 값이 유지됐다. `DeviceIdProduct`는 0x0324로 남았으나 드라이버 선택에 영향 없음.
- 결과: `AppleHSBluetoothDevice` 아래 Trackpad/Actuator/Accelerometer/Device Management 인터페이스, `AppleMultitouchTrackpadHIDEventDriver`와 `AppleMultitouchDevice` 생성. SIP·보안 설정 변경 없음, kext 설치 없음.
- 백업: 같은 디렉터리 `*.tc-backup-*`, `*.tc-before-pid-spoof` (root 0600). 복구: `pid-spoof.py restore` 또는 USB 재연결(재페어링).
- 제스처·Force Click·햅틱 실사용 검증: 진행 중.
