# 반복 승인 없는 로컬 개발 서명

기존 ad-hoc 서명은 앱 식별 요구사항이 실행 파일의 cdhash에 묶였다. 빌드마다 cdhash가 바뀌면 macOS가 이전 손쉬운 사용 승인을 같은 앱에 대한 승인으로 인정하지 않을 수 있다.

TrackpadCompat 전용 개발 인증서와 동일한 bundle identifier로 계속 서명하여 designated requirement를 안정적으로 유지한다. 빌드마다 서명은 수행하지만 사용자가 암호를 입력하는 단계를 자동화한다. 인증서가 바뀌는 최초 전환 때는 손쉬운 사용 승인이 한 번 더 필요할 수 있다. 이후 재빌드 승인 유지 여부는 실제 macOS에서 확인해야 한다.

## 사용자별 자료

인증서, 전용 키체인, 자동 잠금 해제 자료는 `~/Library/Application Support/TrackpadCompat/DevelopmentSigning` 아래에만 둔다. 디렉터리는 0700, 비밀 자료는 0600. 개인키는 전용 키체인에 가져온 뒤 임시 PEM/P12를 삭제한다. 저장소에 비밀 자료를 복사하지 않는다.

- `python3 scripts/setup-local-signing.py`: 최초 1회 전용 인증서 및 키체인 생성
- 인증서는 현재 사용자 영역에서 **codeSign 용도에 한정**하여 신뢰 설정
- `python3 scripts/sign-local.py APP_PATH`: 전용 키체인 잠금 해제, helper와 app 서명·검증, 다시 잠금
- `scripts/build-gui.sh`: 위 자동 서명 사용. 서명 준비가 없으면 실패하며 ad-hoc으로 조용히 대체하지 않음

다른 프로젝트의 인증서 또는 키체인 암호는 사용하지 않는다. 로그인 키체인의 전체 접근 권한이나 시스템 전체 Gatekeeper 설정은 바꾸지 않는다.

## 공개 배포와 구분

이 인증서는 로컬 개발용이다. Apple Developer ID 또는 애플 공증을 대체하지 않는다. 공개 배포의 표준 경로는 별도 Developer ID 서명 및 공증이다.

참고: [Apple TN2206 — Code Designated Requirement](https://developer.apple.com/library/archive/technotes/tn2206/)
