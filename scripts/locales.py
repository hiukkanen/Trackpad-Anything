#!/usr/bin/env python3
"""UI translations (ko source -> en, ja, zh-Hans). Writes GUI/src/locales.json.

Korean source strings are the keys used by t() in the GUI and TCL() in the native host.
Apple's own Trackpad-settings terms are used for ja/zh where they exist.
Run after editing:  python3 scripts/locales.py   (it fails if a key used in code is missing)
"""
import json, re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

T = {
# ---- Sidebar & pages
'포인터 및 스크롤': ('Pointer & Scroll', 'ポインタとスクロール', '指针与滚动'),
'화면 전환': ('Spaces', '画面の切り替え', '切换屏幕'),
'제스처': ('Gestures', 'ジェスチャ', '手势'),
'단축키': ('Shortcuts', 'ショートカット', '快捷键'),
'기기 및 진단': ('Device & Diagnostics', 'デバイスと診断', '设备与诊断'),
'블루투스 연결': ('Bluetooth', 'Bluetooth接続', '蓝牙连接'),
'언어': ('Language', '言語', '语言'),
'시스템 언어': ('System language', 'システムの言語', '系统语言'),
'연결됨 · {0}': ('Connected · {0}', '接続済み · {0}', '已连接 · {0}'),
' · 제스처': (' · Gestures', ' · ジェスチャ', ' · 手势'),
' · 제스처 꺼짐': (' · Gestures off', ' · ジェスチャオフ', ' · 手势已关闭'),
'연결되지 않음': ('Not connected', '未接続', '未连接'),
'배터리 {0}%': ('Battery {0}%', 'バッテリー {0}%', '电池 {0}%'),
'배터리 잔량 확인 중': ('Checking battery', 'バッテリー残量を確認中', '正在检查电量'),
'브라우저 미리보기 · 기기 제어 없음': ('Browser preview · no device control', 'ブラウザプレビュー · デバイス制御なし', '浏览器预览 · 无设备控制'),

# ---- Pointer & scroll
'속도 및 클릭': ('Speed & Click', '速さとクリック', '速度与点按'),
'기본값 복원': ('Restore Defaults', 'デフォルトに戻す', '恢复默认'),
'포인터 속도': ('Pointer speed', 'ポインタの速さ', '指针速度'),
'스크롤 속도': ('Scroll speed', 'スクロールの速さ', '滚动速度'),
'탭하여 클릭': ('Tap to click', 'タップでクリック', '轻点来点按'),
'자연스러운 스크롤': ('Natural scrolling', 'ナチュラルなスクロール', '自然滚动'),
'블루투스 연결 중 · 설정 미러로 macOS 트랙패드 설정에 바로 반영됩니다': ('On Bluetooth · changes go straight to macOS trackpad settings via the settings mirror', 'Bluetooth接続中 · 設定ミラーでmacOSのトラックパッド設定にすぐ反映されます', '蓝牙连接中 · 通过设置镜像立即应用到 macOS 触控板设置'),
'블루투스 연결 중 · 이 값은 USB 연결에만 적용됩니다. 블루투스에도 쓰려면 블루투스 연결에서 설정 미러를 켜세요': ('On Bluetooth · these values apply to USB only. To use them over Bluetooth, turn on the settings mirror on the Bluetooth page', 'Bluetooth接続中 · この値はUSB接続にのみ適用されます。Bluetoothにも使うには、Bluetooth接続ページで設定ミラーをオンにしてください', '蓝牙连接中 · 这些值仅适用于 USB 连接。若要在蓝牙下使用，请在“蓝牙连接”中打开设置镜像'),
'변경한 값은 다음 손가락 동작부터 적용됩니다': ('Changes apply from your next gesture', '変更は次の指の操作から適用されます', '更改将从下一次手势开始生效'),
'설정 적용 시작 후 입력에 반영됩니다': ('Applied to input after you start the engine', 'エンジン開始後に入力へ反映されます', '启动引擎后应用到输入'),
'찾아보기와 세 손가락 드래그는 동시에 사용할 수 없습니다. 하나를 켜면 다른 기능은 꺼집니다.': ('Look up and three‑finger drag can’t be used together. Turning one on turns the other off.', '調べる機能と3本指のドラッグは同時に使えません。一方をオンにするともう一方はオフになります。', '“查询”和“三指拖移”不能同时使用。打开其中一个会关闭另一个。'),
'기존 입력 앱 사용 중': ('Using the previous input app', '以前の入力アプリを使用中', '正在使用旧版输入应用'),
'입력 제어 시작 전': ('Input engine not started', '入力エンジン未開始', '输入引擎尚未启动'),
'설정을 적용하려면 새 입력 엔진으로 전환하세요. 기존 앱은 복구용으로 보관됩니다.': ('Switch to the new input engine to apply settings. The previous app is kept for recovery.', '設定を適用するには新しい入力エンジンに切り替えてください。以前のアプリは復元用に保管されます。', '切换到新的输入引擎以应用设置。旧版应用会保留以便恢复。'),
'연결 중…': ('Connecting…', '接続中…', '正在连接…'),
'설정 적용 시작': ('Start Engine', 'エンジンを開始', '启动引擎'),
'검증 중': ('In testing', '検証中', '测试中'),
'항상 사용': ('Always on', '常にオン', '始终开启'),

# ---- Gesture groups, features and gestures
'포인터 및 클릭': ('Point & Click', 'ポイントとクリック', '光标与点按'),
'스크롤 및 확대': ('Scroll & Zoom', 'スクロールとズーム', '滚动缩放'),
'추가 제스처': ('More Gestures', 'その他のジェスチャ', '更多手势'),
'손쉬운 사용': ('Accessibility', 'アクセシビリティ', '辅助功能'),
'압력 및 햅틱': ('Pressure & Haptics', '圧力と触覚', '压力与触感'),
'포인터 이동': ('Move pointer', 'ポインタの移動', '移动指针'),
'클릭 및 클릭 드래그': ('Click & click‑drag', 'クリックとドラッグ', '点按与拖移'),
'보조 클릭': ('Secondary click', '副ボタンのクリック', '辅助点按'),
'찾아보기 및 데이터 탐색': ('Look up & data detectors', '調べる＆データ検出', '查询与数据检测器'),
'스크롤': ('Scroll', 'スクロール', '滚动'),
'관성 스크롤': ('Scroll with inertia', '慣性スクロール', '惯性滚动'),
'확대 또는 축소': ('Zoom in or out', '拡大／縮小', '放大或缩小'),
'스마트 확대/축소': ('Smart zoom', 'スマートズーム', '智能缩放'),
'회전': ('Rotate', '回転', '旋转'),
'페이지 쓸어넘기기': ('Swipe between pages', 'ページ間をスワイプ', '在页面之间轻扫'),
'전체 화면 앱·데스크탑 전환': ('Swipe between full‑screen apps', 'フルスクリーンアプリケーション間をスワイプ', '在全屏幕应用程序之间轻扫'),
'앱 윈도우': ('App Exposé', 'アプリケーションExposé', '应用程序 Exposé'),
'데스크탑 보기': ('Show Desktop', 'デスクトップを表示', '显示桌面'),
'알림 센터': ('Notification Center', '通知センター', '通知中心'),
'세 손가락 드래그': ('Three‑finger drag', '3本指のドラッグ', '三指拖移'),
'세게 클릭 및 햅틱 피드백': ('Force Click & haptic feedback', '強めのクリックと触覚フィードバック', '用力点按和触感反馈'),
'클릭 강도': ('Click pressure', 'クリックの強さ', '点按力度'),
'한 손가락': ('One finger', '1本指', '单指'),
'누르기': ('Press', '押す', '按下'),
'한 손가락 탭': ('Tap with one finger', '1本指でタップ', '单指轻点'),
'두 손가락 클릭·탭': ('Click or tap with two fingers', '2本指でクリック／タップ', '双指点按或轻点'),
'세 손가락 탭 / 세게 클릭': ('Tap with three fingers / Force Click', '3本指でタップ／強めのクリック', '三指轻点 / 用力点按'),
'두 손가락 이동': ('Two fingers', '2本指で移動', '双指移动'),
'손가락 이동 방향': ('Content tracks finger movement', 'コンテンツが指の動きを追跡', '内容跟随手指移动'),
'손을 뗀 뒤 감속': ('Keeps gliding after you lift', '指を離した後に減速', '抬起手指后继续滑动'),
'두 손가락 벌리기·오므리기': ('Pinch with two fingers', '2本指でピンチ', '双指捏合'),
'두 손가락 이중 탭': ('Double‑tap with two fingers', '2本指でダブルタップ', '双指连按两次'),
'두 손가락 회전': ('Rotate with two fingers', '2本指で回転', '双指旋转'),
'두 손가락 좌우': ('Swipe left or right with two fingers', '2本指で左右にスワイプ', '双指向左或向右轻扫'),
'네 손가락 좌우': ('Swipe left or right with four fingers', '4本指で左右にスワイプ', '四指向左或向右轻扫'),
'네 손가락 위로': ('Swipe up with four fingers', '4本指で上にスワイプ', '四指向上轻扫'),
'네 손가락 아래로': ('Swipe down with four fingers', '4本指で下にスワイプ', '四指向下轻扫'),
'엄지와 세 손가락 오므리기': ('Pinch with thumb and three fingers', '親指と3本指でピンチ', '用拇指和三指捏合'),
'엄지와 세 손가락 벌리기': ('Spread with thumb and three fingers', '親指と3本指で広げる', '用拇指和三指张开'),
'오른쪽 가장자리에서 두 손가락 쓸기': ('Swipe left from the right edge with two fingers', '右端から2本指で左にスワイプ', '双指从右边缘向左轻扫'),
'세 손가락 이동': ('Move with three fingers', '3本指で移動', '三指移动'),
'더 깊게 누르기': ('Press harder', 'さらに押し込む', '用力按下'),
'가볍게 · 중간 · 강하게': ('Light · Medium · Firm', '弱い · 中 · 強い', '轻 · 中 · 重'),
' · 세 손가락 드래그와 선택 사용, ⌃⌘D 방식': (' · Pick this or three‑finger drag; uses ⌃⌘D', ' · 3本指のドラッグとどちらかを選択、⌃⌘D方式', ' · 与三指拖移二选一，使用 ⌃⌘D'),
' · 켜면 두 손가락 좌우 스크롤 대신 페이지 이동': (' · When on, two‑finger horizontal scroll changes pages', ' · オンにすると2本指の左右スクロールでページを移動', ' · 打开后，双指左右滚动改为翻页'),
' · 메뉴 막대 시계 접근 방식': (' · Opens via the menu bar clock', ' · メニューバーの時計から開きます', ' · 通过菜单栏时钟打开'),
' · 압력 단계·햅틱 제어 검증 필요': (' · Pressure stages and haptics need verification', ' · 圧力段階と触覚制御は検証が必要', ' · 压力级别和触感控制有待验证'),
'제스처를 켜거나 끄세요. 새로 추가된 동작은 처음에는 꺼져 있습니다': ('Turn gestures on or off. Newly added gestures start off.', 'ジェスチャのオン／オフを切り替えます。新しく追加された操作は最初はオフです', '打开或关闭手势。新增的手势默认关闭'),

# ---- Spaces page
'은색 Magic Trackpad': ('Silver Magic Trackpad', 'シルバーのMagic Trackpad', '银色 Magic Trackpad'),
'데스크탑 배경 예시': ('Example desktop wallpaper', 'デスクトップ壁紙の例', '桌面墙纸示例'),
'데스크탑 1': ('Desktop 1', 'デスクトップ1', '桌面 1'),
'전체 화면 앱을 포함해 순서대로 전환 · 화면 예시': ('Switches in order, full‑screen apps included · example screens', 'フルスクリーンアプリを含めて順に切り替え · 画面例', '按顺序切换（包括全屏幕应用）· 示例画面'),
'네 손가락으로, 화면 사이를 자유롭게': ('Four fingers, any screen', '4本指で、画面のあいだを自由に', '四指轻扫，自由切换屏幕'),
'원격 연결 중에도 이 Mac에서 전환': ('Switches on this Mac, even inside remote sessions', 'リモート接続中でもこのMacで切り替え', '即使在远程连接中，也在这台 Mac 上切换'),
'이 Mac에서 전환': ('Switch on this Mac', 'このMacで切り替え', '在这台 Mac 上切换'),
'네 손가락 좌우 쓸기': ('Four‑finger left/right swipe', '4本指で左右にスワイプ', '四指左右轻扫'),
'위로 쓸어 Mission Control': ('Swipe up for Mission Control', '上にスワイプでMission Control', '向上轻扫打开调度中心'),
'키보드 단축키': ('Keyboard shortcut', 'キーボードショートカット', '键盘快捷键'),
'이전 데스크탑': ('Previous desktop', '前のデスクトップ', '上一个桌面'),
'다음 데스크탑': ('Next desktop', '次のデスクトップ', '下一个桌面'),
'Mission Control 열기': ('Open Mission Control', 'Mission Controlを開く', '打开调度中心'),
'Parsec과 Jump Desktop의 전체 화면에서도 로컬 데스크탑을 전환합니다': ('Switches local desktops even in full‑screen Parsec and Jump Desktop', 'ParsecやJump Desktopのフルスクリーン中でもローカルのデスクトップを切り替えます', '即使在全屏幕 Parsec 和 Jump Desktop 中也能切换本地桌面'),
'현재 동작': ('Current gestures', '現在の操作', '当前手势'),

# ---- Shortcuts page
'로컬 데스크탑 전환': ('Switch local desktops', 'ローカルデスクトップの切り替え', '切换本地桌面'),
'기존 Karabiner 설정을 유지합니다. GUI에서 단축키 변경은 아직 제공하지 않습니다.': ('Your existing Karabiner setup is kept. Changing shortcuts in the app isn’t available yet.', '既存のKarabiner設定はそのまま使えます。アプリ内でのショートカット変更はまだ対応していません。', '保留现有的 Karabiner 设置。暂不支持在应用中更改快捷键。'),

# ---- Device & diagnostics page
'입력 엔진': ('Input engine', '入力エンジン', '输入引擎'),
'설정 가능한 입력 엔진 실행 중': ('Configurable input engine running', '設定可能な入力エンジンが実行中', '可配置的输入引擎正在运行'),
'이전 앱(0.2) 실행 중': ('Previous app (0.2) running', '以前のアプリ（0.2）が実行中', '旧版应用（0.2）正在运行'),
'연결 대기': ('Waiting', '接続待ち', '等待连接'),
'USB 전송': ('USB transport', 'USB転送', 'USB 传输'),
'USB 연결됨': ('USB connected', 'USB接続済み', 'USB 已连接'),
'미연결': ('Not connected', '未接続', '未连接'),
'연결됨 · macOS 기본 드라이버로 제스처 동작': ('Connected · gestures via macOS’s own driver', '接続済み · macOS標準ドライバでジェスチャが動作', '已连接 · 通过 macOS 自带驱动使用手势'),
'연결됨 · 제스처 꺼짐 (블루투스 연결 메뉴에서 켜기)': ('Connected · gestures off (turn on in Bluetooth)', '接続済み · ジェスチャオフ（Bluetooth接続でオン）', '已连接 · 手势已关闭（在“蓝牙连接”中打开）'),
'로그인 시 자동 실행': ('Open at login', 'ログイン時に開く', '登录时打开'),
'Mac에 로그인하면 메뉴 막대에서 바로 시작합니다': ('Starts in the menu bar when you log in', 'Macにログインするとメニューバーですぐに起動します', '登录 Mac 后在菜单栏中自动启动'),
'상태 새로고침': ('Refresh Status', '状態を更新', '刷新状态'),
'수신 프레임: {0} · 최근 동작: {1}': ('Frames received: {0} · Last action: {1}', '受信フレーム: {0} · 最近の操作: {1}', '已接收帧: {0} · 最近操作: {1}'),
'대기': ('Idle', '待機', '空闲'),
'USB 다시 연결': ('Reconnect USB', 'USBを再接続', '重新连接 USB'),
'이전 앱(0.2)으로 복구': ('Revert to Previous App (0.2)', '以前のアプリ（0.2）に戻す', '恢复为旧版应用（0.2）'),
'입력 중 창을 닫아도 메뉴 막대에서 계속 동작합니다.': ('Keeps running in the menu bar when you close this window.', 'ウインドウを閉じてもメニューバーで動作し続けます。', '关闭此窗口后仍会在菜单栏中继续运行。'),

# ---- Bluetooth page
'블루투스 제스처 사용 중': ('Bluetooth gestures on', 'Bluetoothジェスチャ使用中', '蓝牙手势已启用'),
'macOS에 내장된 Magic Trackpad 드라이버가 이 트랙패드를 인식했습니다. 제스처, 세게 클릭, 햅틱, 시스템 설정의 트랙패드 패널을 그대로 쓸 수 있습니다.': ('macOS’s built‑in Magic Trackpad driver recognized this trackpad. Gestures, Force Click, haptics and the Trackpad pane in System Settings work as usual.', 'macOS内蔵のMagic Trackpadドライバがこのトラックパッドを認識しました。ジェスチャ、強めのクリック、触覚、システム設定のトラックパッドパネルがそのまま使えます。', 'macOS 内建的 Magic Trackpad 驱动已识别此触控板。手势、用力点按、触感反馈和系统设置中的触控板面板均可正常使用。'),
'블루투스 제스처 꺼짐': ('Bluetooth gestures off', 'Bluetoothジェスチャオフ', '蓝牙手势已关闭'),
'macOS 14가 이 모델을 일반 마우스로 인식하고 있습니다. 아래 버튼으로 켤 수 있습니다.': ('macOS 14 sees this model as a plain mouse. Turn gestures on with the button below.', 'macOS 14はこのモデルを通常のマウスとして認識しています。下のボタンでオンにできます。', 'macOS 14 将此型号识别为普通鼠标。可使用下方按钮打开手势。'),
'USB로 연결됨': ('Connected via USB', 'USBで接続済み', '已通过 USB 连接'),
'USB에서는 Trackpad Anything 입력 엔진이 동작합니다. 블루투스 설정은 케이블을 뽑고 블루투스로 연결한 뒤 할 수 있습니다.': ('Over USB, the Trackpad Anything input engine is used. To set up Bluetooth, unplug the cable and connect over Bluetooth.', 'USBではTrackpad Anythingの入力エンジンが動作します。Bluetoothの設定は、ケーブルを抜いてBluetoothで接続してから行えます。', '通过 USB 时使用 Trackpad Anything 输入引擎。要设置蓝牙，请拔下线缆并通过蓝牙连接。'),
'트랙패드가 연결되지 않음': ('Trackpad not connected', 'トラックパッドが接続されていません', '触控板未连接'),
'트랙패드를 켜고 블루투스로 연결하세요.': ('Turn on the trackpad and connect it over Bluetooth.', 'トラックパッドの電源を入れ、Bluetoothで接続してください。', '请打开触控板并通过蓝牙连接。'),
'블루투스 제스처': ('Bluetooth Gestures', 'Bluetoothジェスチャ', '蓝牙手势'),
'진행 중…': ('Working…', '処理中…', '正在处理…'),
'블루투스 제스처 켜기': ('Enable Bluetooth Gestures', 'Bluetoothジェスチャをオン', '启用蓝牙手势'),
'원래대로 되돌리기': ('Restore', '元に戻す', '恢复原状'),
'관리자 암호를 한 번 묻습니다. 적용하는 동안 블루투스 키보드·마우스·헤드폰이 몇 초 끊겼다가 다시 연결됩니다.': ('You’ll be asked for an administrator password once. Bluetooth keyboards, mice and headphones disconnect for a few seconds while it applies.', '管理者パスワードを一度だけ求めます。適用中はBluetoothのキーボード、マウス、ヘッドフォンが数秒切断され、再接続されます。', '将要求输入一次管理员密码。应用期间，蓝牙键盘、鼠标和耳机会断开几秒钟后重新连接。'),
'설정 미러': ('Settings Mirror', '設定ミラー', '设置镜像'),
'블루투스 제스처가 동작하는 동안, 이 앱의 포인터·스크롤·제스처 설정을 macOS 트랙패드 설정에 그대로 씁니다. 앱 하나로 USB와 블루투스를 모두 조절할 수 있습니다.': ('While Bluetooth gestures are on, this app’s pointer, scroll and gesture settings are written to macOS’s trackpad settings, so one app controls both USB and Bluetooth.', 'Bluetoothジェスチャの動作中、このアプリのポインタ・スクロール・ジェスチャ設定をmacOSのトラックパッド設定にそのまま書き込みます。1つのアプリでUSBとBluetoothの両方を調整できます。', '蓝牙手势启用期间，此应用的指针、滚动和手势设置会写入 macOS 触控板设置。一个应用即可同时调节 USB 和蓝牙。'),
'macOS 설정은 모든 트랙패드에 공통입니다': ('macOS settings apply to every trackpad', 'macOSの設定はすべてのトラックパッドに共通です', 'macOS 设置对所有触控板生效'),
'켜는 순간 앱 설정값이 macOS에 덮어써지며, MacBook 내장 트랙패드에도 적용됩니다(예: 포인터 속도, 회전, 관성 스크롤). 처음 켤 때 기존 macOS 값을 백업하므로 언제든 되돌릴 수 있습니다.': ('Turning this on overwrites macOS’s values with the app’s, including for a MacBook’s built‑in trackpad (e.g. pointer speed, rotate, inertia). Your previous macOS values are backed up the first time, so you can always restore them.', 'オンにした時点でアプリの設定値がmacOSに上書きされ、MacBook内蔵トラックパッドにも適用されます（例：ポインタの速さ、回転、慣性スクロール）。最初にオンにしたときに既存のmacOSの値をバックアップするので、いつでも元に戻せます。', '打开后，应用的设置会覆盖 macOS 的设置，并同样作用于 MacBook 内建触控板（例如指针速度、旋转、惯性滚动）。首次打开时会备份原有 macOS 设置，可随时恢复。'),
'macOS 설정 원래대로': ('Restore macOS Settings', 'macOSの設定を元に戻す', '恢复 macOS 设置'),
'백업 위치': ('Backups', 'バックアップの場所', '备份位置'),
'블루투스 기록 원본': ('Original Bluetooth record', '元のBluetooth記録', '原始蓝牙记录'),
'관리자 전용': ('Admin only', '管理者のみ', '仅限管理员'),
'macOS 트랙패드 설정': ('macOS trackpad settings', 'macOSのトラックパッド設定', 'macOS 触控板设置'),
'설정 미러를 처음 켤 때 만들어집니다': ('Created the first time you turn on the settings mirror', '設定ミラーを初めてオンにしたときに作成されます', '首次打开设置镜像时创建'),
'Finder에서 보기': ('Show in Finder', 'Finderで表示', '在访达中显示'),
'동작 원리': ('How It Works', '仕組み', '工作原理'),
'1. 왜 안 됐나': ('1. Why it didn’t work', '1. なぜ動かなかったか', '1. 为什么不能用'),
'macOS 14에는 USB-C 모델(제품 ID 0x0324)용 블루투스 드라이버가 없어 일반 마우스로 인식합니다. 터치 데이터는 앱에 도착하기 전에 버려집니다.': ('macOS 14 has no Bluetooth driver for the USB‑C model (product ID 0x0324), so it’s treated as a plain mouse. Touch data is dropped before any app can see it.', 'macOS 14にはUSB-Cモデル（製品ID 0x0324）用のBluetoothドライバがないため、通常のマウスとして認識されます。タッチデータはアプリに届く前に破棄されます。', 'macOS 14 没有适用于 USB-C 型号（产品 ID 0x0324）的蓝牙驱动，因此会被识别为普通鼠标。触控数据在到达应用之前就被丢弃。'),
'2. 무엇을 바꾸나': ('2. What changes', '2. 何を変えるか', '2. 更改了什么'),
'Mac의 블루투스 기록에 저장된 이 트랙패드의 제품 ID만 Lightning 모델(0x0265)로 바꿉니다. 트랙패드 펌웨어, macOS 시스템 파일, SIP 같은 보안 설정, 페어링 키는 바꾸지 않습니다.': ('Only this trackpad’s product ID in the Mac’s Bluetooth record is changed, to the Lightning model’s (0x0265). Firmware, macOS system files, security settings like SIP and pairing keys are left alone.', 'Macに保存されたBluetooth記録のうち、このトラックパッドの製品IDだけをLightningモデル（0x0265）に変更します。トラックパッドのファームウェア、macOSのシステムファイル、SIPなどのセキュリティ設定、ペアリングキーは変更しません。', '只将 Mac 蓝牙记录中此触控板的产品 ID 改为 Lightning 型号（0x0265）。不会更改触控板固件、macOS 系统文件、SIP 等安全设置或配对密钥。'),
'3. 왜 동작하나': ('3. Why it works', '3. なぜ動くのか', '3. 为什么可行'),
'두 모델은 블루투스 통신 방식이 같아서, macOS에 이미 들어 있는 Magic Trackpad 2 드라이버가 그대로 동작합니다.': ('Both models use the same Bluetooth protocol, so the Magic Trackpad 2 driver already in macOS just works.', '2つのモデルはBluetoothの通信方式が同じなので、macOSにすでにあるMagic Trackpad 2ドライバがそのまま動作します。', '两种型号的蓝牙通信方式相同，因此 macOS 自带的 Magic Trackpad 2 驱动可以直接使用。'),
'4. 적용 방식': ('4. How it’s applied', '4. 適用方法', '4. 如何应用'),
'기록을 바꾼 뒤 블루투스 서비스를 다시 시작해 macOS가 새 기록을 읽게 합니다. 기록 형식이 예상과 다르면 아무것도 바꾸지 않고 멈춥니다.': ('After the change, the Bluetooth service restarts so macOS reads the new record. If the record format isn’t what’s expected, nothing is changed.', '記録を変更した後、Bluetoothサービスを再起動してmacOSに新しい記録を読み込ませます。記録の形式が想定と異なる場合は何も変更せずに中止します。', '更改后会重启蓝牙服务，让 macOS 读取新记录。如果记录格式与预期不符，则不做任何更改并停止。'),
'알아둘 점': ('Good to Know', '知っておきたいこと', '注意事项'),
'케이블로 Mac에 연결하면 풀립니다': ('Plugging into this Mac undoes it', 'ケーブルでMacに接続すると解除されます', '用线缆连接这台 Mac 会还原'),
'다시 페어링되며 원래 ID로 돌아갑니다. 충전은 일반 USB-C 충전기를 쓰거나, 충전 후 이 화면에서 다시 켜세요.': ('It re‑pairs and goes back to the original ID. Charge with a regular USB‑C charger, or turn it on again here afterwards.', '再ペアリングされ、元のIDに戻ります。充電は一般的なUSB-C充電器を使うか、充電後にこの画面で再度オンにしてください。', '会重新配对并恢复为原始 ID。请使用普通 USB-C 充电器充电，或在充电后于此页面重新启用。'),
'시스템 정보 표시': ('System Information', 'システム情報の表示', '系统信息显示'),
'시스템 정보에는 계속 0x0324로 보일 수 있습니다. 동작에는 영향이 없습니다.': ('System Information may still show 0x0324. That doesn’t affect anything.', 'システム情報には引き続き0x0324と表示される場合があります。動作には影響しません。', '系统信息中可能仍显示 0x0324。这不影响使用。'),
'출처': ('Credits', '出典', '来源'),
'아이디어: GianMarioBen/MagicTrackPadCatalina (Catalina용). Trackpad Anything은 Sonoma 기록 형식에 맞춰 독립적으로 구현했습니다.': ('Idea: GianMarioBen/MagicTrackPadCatalina (for Catalina). Trackpad Anything is an independent implementation for the Sonoma record format.', 'アイデア：GianMarioBen/MagicTrackPadCatalina（Catalina用）。Trackpad AnythingはSonomaの記録形式に合わせて独自に実装しました。', '思路来源：GianMarioBen/MagicTrackPadCatalina（适用于 Catalina）。Trackpad Anything 针对 Sonoma 的记录格式独立实现。'),

# ---- Menu bar
'Magic Trackpad 확인 중': ('Checking Magic Trackpad', 'Magic Trackpadを確認中', '正在检查 Magic Trackpad'),
'Trackpad Anything 설정 열기': ('Open Trackpad Anything Settings', 'Trackpad Anythingの設定を開く', '打开 Trackpad Anything 设置'),
'USB 트랙패드 다시 연결': ('Reconnect USB Trackpad', 'USBトラックパッドを再接続', '重新连接 USB 触控板'),
'종료': ('Quit', '終了', '退出'),
'Trackpad Anything, 배터리 {0}%': ('Trackpad Anything, battery {0}%', 'Trackpad Anything、バッテリー {0}%', 'Trackpad Anything，电池 {0}%'),
'Trackpad Anything · 배터리 {0}%': ('Trackpad Anything · Battery {0}%', 'Trackpad Anything · バッテリー {0}%', 'Trackpad Anything · 电池 {0}%'),
'Bluetooth · macOS 제스처': ('Bluetooth · macOS gestures', 'Bluetooth · macOSジェスチャ', '蓝牙 · macOS 手势'),
'Bluetooth · 제스처 꺼짐': ('Bluetooth · gestures off', 'Bluetooth · ジェスチャオフ', '蓝牙 · 手势已关闭'),
'Magic Trackpad 연결되지 않음': ('Magic Trackpad not connected', 'Magic Trackpad未接続', 'Magic Trackpad 未连接'),

# ---- Notices from the native host
'실행: {0}': ('Ran: {0}', '実行: {0}', '已执行: {0}'),
'동작 실패: {0}': ('Action failed: {0}', '操作に失敗: {0}', '操作失败: {0}'),
'이전 앱(0.2)이 실행 중입니다. 손쉬운 사용 허용 후 적용 시작을 누르면 하나로 합쳐집니다': ('The previous app (0.2) is running. Allow Accessibility, then click Start Engine to merge them.', '以前のアプリ（0.2）が実行中です。アクセシビリティを許可してからエンジンを開始すると1つにまとまります', '旧版应用（0.2）正在运行。允许辅助功能后点按“启动引擎”即可合并。'),
'기존 입력 앱을 다시 열었습니다': ('Reopened the previous input app', '以前の入力アプリを再度開きました', '已重新打开旧版输入应用'),
'손쉬운 사용에서 Trackpad Anything을 허용한 뒤 적용 시작을 눌러 주세요': ('Allow Trackpad Anything in Accessibility, then click Start Engine', 'アクセシビリティでTrackpad Anythingを許可してから、エンジンを開始してください', '请在“辅助功能”中允许 Trackpad Anything，然后点按“启动引擎”'),
'입력 엔진 전환 중': ('Switching input engine', '入力エンジンを切り替え中', '正在切换输入引擎'),
'기존 앱이 종료되지 않아 전환을 중단했습니다': ('The previous app didn’t quit, so switching stopped', '以前のアプリが終了しなかったため切り替えを中止しました', '旧版应用未退出，已停止切换'),
'기존 USB 연결이 종료되지 않았습니다. 다시 적용하거나 기존 앱으로 복구하세요': ('The previous USB connection is still open. Try again or revert to the previous app.', '以前のUSB接続が終了していません。再度適用するか、以前のアプリに戻してください', '旧的 USB 连接尚未关闭。请重试或恢复为旧版应用'),
'입력 엔진 종료 ({0})': ('Input engine stopped ({0})', '入力エンジン終了（{0}）', '输入引擎已停止（{0}）'),
'입력 엔진을 시작하지 못했습니다': ('Couldn’t start the input engine', '入力エンジンを開始できませんでした', '无法启动输入引擎'),
'블루투스: macOS 기본 드라이버로 동작 중입니다': ('Bluetooth: running on macOS’s own driver', 'Bluetooth: macOS標準ドライバで動作中です', '蓝牙：正在使用 macOS 自带驱动'),
'입력 엔진 준비 완료 · USB 연결 시 자동으로 연결합니다': ('Input engine ready · connects automatically when you plug in USB', '入力エンジン準備完了 · USB接続時に自動で接続します', '输入引擎已就绪 · 连接 USB 时自动连接'),
'USB로 연결됐습니다. 케이블 연결로 재페어링되면 블루투스 제스처를 다시 켜야 할 수 있습니다': ('Connected via USB. If the cable re‑paired the trackpad, you may need to enable Bluetooth gestures again.', 'USBで接続しました。ケーブル接続で再ペアリングされた場合は、Bluetoothジェスチャを再度オンにする必要があります', '已通过 USB 连接。如果线缆连接导致重新配对，可能需要重新启用蓝牙手势'),
'USB 연결이 종료되었습니다. USB 다시 연결 또는 기존 앱 복구를 선택하세요': ('USB connection ended. Reconnect USB or revert to the previous app.', 'USB接続が終了しました。USBを再接続するか、以前のアプリに戻してください', 'USB 连接已结束。请重新连接 USB 或恢复为旧版应用'),
'USB 연결을 위한 관리자 인증을 완료해 주세요': ('Please authenticate as an administrator to connect USB', 'USB接続のため管理者認証を完了してください', '请完成管理员认证以连接 USB'),
'macOS 설정 백업에 실패해 미러를 적용하지 않았습니다': ('Couldn’t back up macOS settings, so the mirror wasn’t applied', 'macOS設定のバックアップに失敗したため、ミラーを適用しませんでした', '备份 macOS 设置失败，未应用镜像'),
'macOS 트랙패드 설정 {0}개 항목에 반영했습니다': ('Applied to {0} macOS trackpad settings', 'macOSのトラックパッド設定 {0} 項目に反映しました', '已应用到 {0} 项 macOS 触控板设置'),
'macOS 트랙패드 설정이 이미 같습니다': ('macOS trackpad settings already match', 'macOSのトラックパッド設定はすでに同じです', 'macOS 触控板设置已一致'),
'관리자 인증을 완료해 주세요 · 블루투스 기기가 몇 초간 끊깁니다': ('Please authenticate as an administrator · Bluetooth devices will disconnect for a few seconds', '管理者認証を完了してください · Bluetoothデバイスが数秒間切断されます', '请完成管理员认证 · 蓝牙设备将断开几秒钟'),
'블루투스 제스처를 켰습니다. 트랙패드가 다시 연결되면 macOS 기본 제스처가 동작합니다': ('Bluetooth gestures enabled. Once the trackpad reconnects, macOS gestures work.', 'Bluetoothジェスチャをオンにしました。トラックパッドが再接続されるとmacOS標準のジェスチャが動作します', '已启用蓝牙手势。触控板重新连接后即可使用 macOS 手势'),
'원래 상태로 되돌렸습니다. 블루투스에서는 기본 마우스로만 동작합니다': ('Restored. Over Bluetooth it works as a basic mouse only.', '元の状態に戻しました。Bluetoothでは基本的なマウスとしてのみ動作します', '已恢复原状。通过蓝牙时仅作为基本鼠标使用'),
'이미 블루투스 제스처가 켜져 있습니다': ('Bluetooth gestures are already on', 'Bluetoothジェスチャはすでにオンです', '蓝牙手势已启用'),
'이미 원래 상태입니다': ('Already in the original state', 'すでに元の状態です', '已是原始状态'),
'블루투스 기록에서 트랙패드를 찾지 못했습니다. 블루투스로 한 번 연결한 뒤 다시 시도하세요': ('Couldn’t find the trackpad in the Bluetooth record. Connect it over Bluetooth once and try again.', 'Bluetooth記録にトラックパッドが見つかりません。一度Bluetoothで接続してから再度お試しください', '在蓝牙记录中找不到触控板。请先通过蓝牙连接一次后再试'),
'이 macOS의 블루투스 기록 형식이 예상과 달라 아무것도 바꾸지 않았습니다': ('This macOS uses an unexpected Bluetooth record format, so nothing was changed', 'このmacOSのBluetooth記録形式が想定と異なるため、何も変更しませんでした', '此 macOS 的蓝牙记录格式与预期不同，未做任何更改'),
'macOS가 기록을 되돌렸습니다. 트랙패드 전원을 껐다 켠 뒤 다시 시도하세요': ('macOS reverted the record. Turn the trackpad off and on, then try again.', 'macOSが記録を元に戻しました。トラックパッドの電源を入れ直してから再度お試しください', 'macOS 已还原记录。请关闭再打开触控板后重试'),
'관리자 인증이 취소되었습니다': ('Administrator authentication was cancelled', '管理者認証がキャンセルされました', '已取消管理员认证'),
'블루투스 제스처 켜기에 실패했습니다': ('Couldn’t enable Bluetooth gestures', 'Bluetoothジェスチャをオンにできませんでした', '无法启用蓝牙手势'),
'되돌리기에 실패했습니다': ('Couldn’t restore', '元に戻せませんでした', '无法恢复'),
'연결됨 · 설정이 입력에 적용됩니다': ('Connected · settings apply to input', '接続済み · 設定が入力に適用されます', '已连接 · 设置将应用到输入'),
'기존 사용감의 기본값으로 복원했습니다': ('Restored the default feel', '既定の操作感に戻しました', '已恢复默认手感'),
'기본값 저장에 실패했습니다': ('Couldn’t save defaults', 'デフォルトの保存に失敗しました', '无法保存默认值'),
'설정을 저장했습니다. 다음 손가락 동작부터 적용됩니다': ('Saved. Applies from your next gesture.', '設定を保存しました。次の指の操作から適用されます', '已保存。将从下一次手势开始生效'),
'설정을 저장했습니다. 적용 시작 후 입력에 반영됩니다': ('Saved. Applies after you start the engine.', '設定を保存しました。エンジン開始後に入力へ反映されます', '已保存。启动引擎后生效'),
'설정 저장에 실패해 이전 값으로 되돌렸습니다': ('Couldn’t save, so the previous value was restored', '設定の保存に失敗したため、以前の値に戻しました', '保存失败，已恢复为之前的值'),
'허용되지 않은 설정값입니다': ('That value isn’t allowed', '許可されていない設定値です', '不允许的设置值'),
'설정 미러를 켰습니다. 블루투스 제스처가 동작할 때 macOS 설정에 반영됩니다': ('Settings mirror on. Changes go to macOS settings while Bluetooth gestures are active.', '設定ミラーをオンにしました。Bluetoothジェスチャの動作中にmacOSの設定に反映されます', '已打开设置镜像。蓝牙手势启用时将应用到 macOS 设置'),
'설정 미러를 껐습니다. macOS 설정은 지금 상태로 유지됩니다': ('Settings mirror off. macOS settings stay as they are.', '設定ミラーをオフにしました。macOSの設定は現在のまま保持されます', '已关闭设置镜像。macOS 设置保持当前状态'),
'macOS 트랙패드 설정을 미러 사용 전 값으로 되돌렸습니다': ('Restored macOS trackpad settings from before the mirror', 'macOSのトラックパッド設定をミラー使用前の値に戻しました', '已将 macOS 触控板设置恢复为使用镜像前的值'),
'되돌릴 macOS 설정 백업이 없습니다': ('There’s no macOS settings backup to restore', '元に戻すmacOS設定のバックアップがありません', '没有可恢复的 macOS 设置备份'),
'로그인할 때 Trackpad Anything을 자동으로 실행합니다': ('Trackpad Anything will open at login', 'ログイン時にTrackpad Anythingを自動で起動します', '登录时将自动打开 Trackpad Anything'),
'로그인 시 자동 실행을 껐습니다': ('Open at login turned off', 'ログイン時の自動起動をオフにしました', '已关闭登录时打开'),
'시스템 설정 > 일반 > 로그인 항목에서 Trackpad Anything을 허용해 주세요': ('Please allow Trackpad Anything in System Settings > General > Login Items', 'システム設定 > 一般 > ログイン項目でTrackpad Anythingを許可してください', '请在“系统设置” > “通用” > “登录项”中允许 Trackpad Anything'),
'로그인 항목 설정에 실패했습니다': ('Couldn’t update login items', 'ログイン項目の設定に失敗しました', '无法更新登录项'),
'입력 소켓 경로가 너무 깁니다': ('The input socket path is too long', '入力ソケットのパスが長すぎます', '输入套接字路径过长'),
'화면 전환 명령을 기존 앱에 전달했습니다': ('Sent the switch command to the input app', '画面切り替えコマンドを入力アプリに送信しました', '已将切换命令发送给输入应用'),
'입력 앱에 연결하지 못했습니다': ('Couldn’t reach the input app', '入力アプリに接続できませんでした', '无法连接输入应用'),
'Mission Control 실행 명령을 보냈습니다': ('Sent the Mission Control command', 'Mission Control実行コマンドを送信しました', '已发送调度中心命令'),
'Mission Control 실행에 실패했습니다': ('Couldn’t open Mission Control', 'Mission Controlを開けませんでした', '无法打开调度中心'),
}


def used_keys():
    keys = set()
    app = (ROOT / 'GUI/src/App.tsx').read_text()
    keys |= set(re.findall(r"t\('((?:[^'\\]|\\.)*)'", app))
    keys |= {k for pair in re.findall(r"step\('([^']*)','([^']*)'\)", app) for k in pair}
    keys |= set(re.findall(r"'([^'\n]*[가-힣][^'\n]*)'", re.search(r"const nav=.*", app).group(0)))
    keys |= set(re.findall(r"'([^']*)'", re.search(r"const groups=\[(.*?)\]", app).group(1)))
    keys |= set(re.findall(r":'([^']*)'", re.search(r"const gestureNote.*", app).group(0)))
    feats = (ROOT / 'GUI/src/features.ts').read_text()
    keys |= set(re.findall(r"name:'([^']*)'", feats)) | set(re.findall(r"gesture:'([^']*)'", feats))
    host = (ROOT / 'Sources/App/GUIHost.m').read_text()
    keys |= set(re.findall(r'TCLf?\(@"([^"]*)"', host))
    return {k for k in keys if re.search('[가-힣]', k)}


if __name__ == '__main__':
    missing = sorted(used_keys() - T.keys())
    if missing:
        sys.exit('missing translations:\n' + '\n'.join(missing))
    for ko, tr in T.items():
        for text in tr:
            if set(re.findall(r'\{\d\}', ko)) != set(re.findall(r'\{\d\}', text)):
                sys.exit(f'placeholder mismatch: {ko!r} -> {text!r}')
    out = {lang: {ko: tr[i] for ko, tr in T.items()} for i, lang in enumerate(('en', 'ja', 'zh'))}
    (ROOT / 'GUI/src/locales.json').write_text(json.dumps(out, ensure_ascii=False, indent=1) + '\n')
    unused = sorted(T.keys() - used_keys())
    print(f'locales.json: {len(T)} strings × 3 languages' + (f' ({len(unused)} unused)' if unused else ''))
