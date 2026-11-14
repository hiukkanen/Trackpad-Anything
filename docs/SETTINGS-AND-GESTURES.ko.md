# 설정 연결 및 제스처 구현 상태

## 기존 동작 보존

설치된 `~/Applications/TrackpadCompat.app`는 수정하지 않는다. 현재 동작 중인 앱은 새 GUI의 **설정 적용 시작**을 사용하기 전까지 계속 실행된다. 새 앱에 손쉬운 사용 권한이 없으면 전환하지 않는다.

- 기존 포인터 배율 0.30, 스크롤 배율 0.22 유지
- 좌우 Dock 제스처 헤더, 필드, 진행률 1/1600, 종료 재전송 그대로 유지
- 네 손가락 위로: -180 임계값과 `com.apple.expose.awake` 직접 호출 유지
- 기존 Karabiner `SPACE_LEFT`, `SPACE_RIGHT` 소켓 명령 호환
- 새 기능은 기본 비활성. 설정 변경은 접촉 중인 제스처가 끝난 다음 적용
- 세 손가락 드래그와 찾아보기는 상호 배타적
- 새 GUI 진단 화면 및 메뉴 막대에서 기존 앱으로 복구 가능

## 실제 연결된 설정

첫 화면 상단: 포인터 속도, 스크롤 속도, 탭하여 클릭, 자연스러운 스크롤, 기본값 복원.

제스처 화면: 보조 클릭, 확대/축소, 스마트 확대/축소, 회전, 관성 스크롤, 페이지 전환, 데스크탑 전환, Mission Control, 앱 윈도우, Launchpad, 데스크탑 보기, 알림 센터, 세 손가락 드래그, 찾아보기.

`Sources/Core/Options.def`가 엔진의 기본값 및 검증 범위 원본이다. 사용자 설정은 `~/Library/Application Support/TrackpadCompat/settings-v1.plist`에 원자적으로 저장한다. 잘못된 키, 범위 밖 수치, NaN, 불리언 대신 분수는 거절한다. GUI가 저장된 실제 값을 돌려받아 표시한다. 기존 앱이 실행 중이면 저장만 되고 아직 적용되지 않았음을 화면에 표시한다.

## 이벤트 경로 및 한계

- 스마트 확대: macOS zoom-toggle 이벤트
- 회전/확대: macOS gesture 이벤트 (앱별 처리 지원 필요)
- 페이지: navigation swipe 이벤트. 켜면 두 손가락 수평 스크롤을 페이지 전환에 할당
- 앱 윈도우 / Launchpad / 데스크탑 보기: Dock notification 직접 호출
- 알림 센터: Control Center의 clock 메뉴 항목에 AXPress. 전체 화면/메뉴 막대 상태별 실기 검증 필요
- 찾아보기: macOS 기본 Control+Command+D를 이용. 원격 앱의 키 캡처 영향을 받을 수 있으며 네이티브 force-click 대체라고 표시하지 않음
- Force Touch 압력 단계, 햅틱 피드백 및 클릭 강도는 **완료하지 않음**. Linux의 보고서 정의에서 압력 바이트를 확인했지만, 압력 임계값 보정 및 하드웨어 햅틱 제어를 검증하지 못했다. 동작하지 않는 스위치를 제공하지 않음
- Bluetooth 전송은 아직 미구현. USB 연결 사용
- Apple Pencil은 제품 범위에서 제외

## 검증

`bash scripts/test.sh`: macOS 이벤트를 실제로 보내지 않고 캡처하여 검사한다.

- 기존 포인터/스크롤 배율 및 좌우 Dock 진행률
- 네 손가락 위로 한 번만 실행, 아래로 앱 윈도우
- 설정 적용, 자연스러운 스크롤 반전
- 확대/축소·회전 이벤트의 시작/변경/종료
- 두 손가락 이중 탭, 페이지 전환, 네 손가락 벌리기/오므리기
- 관성 스크롤 시작 및 새 접촉 시 취소
- 드래그 정리와 마우스 버튼 해제, 잘못된 패킷 거절
- 설정 저장/다시 읽기, 잘못된 값, 저장 실패 처리

`bash scripts/build-gui.sh`: TypeScript, Vite, Astryx doctor, C/Objective-C 및 USB helper 빌드, 앱 서명 검증.

2026-11-14: 위 테스트 통과. GUI에서 포인터 슬라이더를 0.30→0.31→0.30으로 조작해 네이티브 저장과 표시 갱신 확인. 새 빌드의 실제 트랙패드 동작은 권한 부여 및 전환 후 사용자 확인이 필요하다. 테스트 통과를 하드웨어 검증으로 취급하지 않는다.

## 참고 구현

- [WebKit CoreGraphicsTestSPI](https://github.com/WebKit/WebKit/blob/main/Tools/TestRunnerShared/spi/CoreGraphicsTestSPI.h)
- [LinearMouse DockKit](https://github.com/linearmouse/linearmouse/blob/main/Modules/DockKit/Sources/DockKit/DockKit.swift) — MIT
- [Linux hid-magicmouse](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-magicmouse.c) — USB 보고서 필드

