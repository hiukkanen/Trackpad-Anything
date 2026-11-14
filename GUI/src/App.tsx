// Settings-sidebar frame adapted from Astryx, Copyright Meta Platforms, Inc., MIT.
import {useState,useEffect} from 'react';
import {Layout,LayoutPanel,LayoutContent,VStack,HStack,StackItem} from '@astryxdesign/core/Layout';
import {Text,Heading} from '@astryxdesign/core/Text';
import {List,ListItem} from '@astryxdesign/core/List';
import {Button} from '@astryxdesign/core/Button';
import {Divider} from '@astryxdesign/core/Divider';
import {Icon} from '@astryxdesign/core/Icon';
import {ComputerDesktopIcon,HandRaisedIcon,CursorArrowRaysIcon,CommandLineIcon,InformationCircleIcon,SignalIcon} from '@heroicons/react/24/outline';
import {Banner} from '@astryxdesign/core/Banner';
import {features} from './features';
import {Switch} from '@astryxdesign/core/Switch';
import {Slider} from '@astryxdesign/core/Slider';
import {defaults,featureOption,type BridgeStatus} from './settings';
import {t,setLang,languages} from './i18n';

declare global {interface Window {webkit?:{messageHandlers:{trackpad:{postMessage:(body:unknown)=>void}}};tcReceive?:(value:any)=>void}}
const muted='var(--color-text-secondary)',accent='var(--color-accent)';
// Page keys stay Korean (also used by TA_START_PAGE); labels are translated when shown.
const nav=[['포인터 및 스크롤',CursorArrowRaysIcon],['화면 전환',ComputerDesktopIcon],['제스처',HandRaisedIcon],['단축키',CommandLineIcon],['기기 및 진단',InformationCircleIcon],['블루투스 연결',SignalIcon]] as const;
const groups=['포인터 및 클릭','스크롤 및 확대','추가 제스처','손쉬운 사용','압력 및 햅틱'];
const panel={padding:'var(--spacing-6)',borderRadius:'var(--radius-container)',background:'var(--color-background-surface)'};
function send(command:string,body:Record<string,unknown>={}){window.webkit?.messageHandlers.trackpad.postMessage({command,...body});}

function Trackpad(){return <VStack gap={2} style={{position:'relative',width:'100%',maxWidth:'26rem',alignSelf:'center'}}><img src="assets/trackpad.png" alt={t('은색 Magic Trackpad')} style={{width:'100%',display:'block'}}/><svg viewBox="0 0 480 320" aria-hidden="true" style={{position:'absolute',inset:0,width:'100%',height:'100%'}}><path d="M160 130h160m-148-8-12 8 12 8m136-16 12 8-12 8" fill="none" stroke={accent} strokeWidth="3" strokeLinecap="round"/>{[186,222,258,294].map(x=><ellipse key={x} cx={x} cy="175" rx="9" ry="12" fill="var(--color-background-surface)" stroke={muted} strokeWidth="1.2"/>)}</svg></VStack>}
// Original illustrative wallpapers, not screenshots of connected desktops.
function Wallpaper({index}:{index:number}){const palettes=[['#efd9bd','#dfae7e','#bf714b','#824433'],['#bfd6e6','#83b2d4','#417dba','#244a83'],['#c8d3d6','#93a9ad','#637f82','#3f5e60']];const c=palettes[index];return <svg viewBox="0 0 240 155" role="img" aria-label={t('데스크탑 배경 예시')} style={{width:'100%',display:'block',borderRadius:'var(--radius-container)'}}><rect width="240" height="155" fill={c[0]}/><path d={index===2?'M0 120 70 34 118 91 179 19 240 96V155H0Z':'M0 105Q50 18 130 67T240 45V155H0Z'} fill={c[1]}/><path d="M0 134Q73 64 143 98T240 61V155H0Z" fill={c[2]}/><path d="M0 147Q81 113 144 128T240 108V155H0Z" fill={c[3]}/><rect x="0" y="0" width="240" height="9" fill="white" opacity=".3"/><rect x="73" y="143" width="94" height="8" rx="3" fill="white" opacity=".45"/>{[83,95,107,119,131,143,155].map(x=><rect key={x} x={x} y="145" width="5" height="4" rx="1" fill="white" opacity=".8"/>)}</svg>}
function Keys(){return <HStack gap={2}>{['⌃','⌥','⌘','←','→'].map(k=><Text key={k} type="code" style={{background:'var(--color-background-body)',borderRadius:'var(--radius-control)',padding:'var(--spacing-2)',minWidth:'2rem',textAlign:'center'}}>{k}</Text>)}</HStack>}
function SpaceDiagram(){return <VStack gap={4}><HStack gap={3}>{[t('데스크탑 1'),'Parsec','Jump Desktop'].map((name,i)=><StackItem key={name} style={{flex:1,minWidth:0}}><VStack gap={3}><Wallpaper index={i}/><Text type="label" justify="center" color={i===1?'accent':'secondary'}>{name}</Text></VStack></StackItem>)}</HStack><Text type="supporting" justify="center">{t('전체 화면 앱을 포함해 순서대로 전환 · 화면 예시')}</Text><Trackpad/><VStack gap={2}><Text type="label" justify="center">{t('네 손가락으로, 화면 사이를 자유롭게')}</Text><Text type="supporting" justify="center">{t('원격 연결 중에도 이 Mac에서 전환')}</Text></VStack></VStack>}

// Bluetooth setup: product-ID record change so macOS 14's own Magic Trackpad 2 driver claims the USB-C model.
function BluetoothPage({status}:{status:BridgeStatus}){
 const bt=status.bluetooth;const onBT=status.transport==='Bluetooth';const native=!!bt?.native;const busy=!!bt?.busy;
 const state:{s:'success'|'warning'|'info';t:string;d:string}=onBT?(native
  ?{s:'success',t:t('블루투스 제스처 사용 중'),d:t('macOS에 내장된 Magic Trackpad 드라이버가 이 트랙패드를 인식했습니다. 제스처, 세게 클릭, 햅틱, 시스템 설정의 트랙패드 패널을 그대로 쓸 수 있습니다.')}
  :{s:'warning',t:t('블루투스 제스처 꺼짐'),d:t('macOS 14가 이 모델을 일반 마우스로 인식하고 있습니다. 아래 버튼으로 켤 수 있습니다.')})
  :status.transport==='USB'?{s:'info',t:t('USB로 연결됨'),d:t('USB에서는 Trackpad Anything 입력 엔진이 동작합니다. 블루투스 설정은 케이블을 뽑고 블루투스로 연결한 뒤 할 수 있습니다.')}
  :{s:'info',t:t('트랙패드가 연결되지 않음'),d:t('트랙패드를 켜고 블루투스로 연결하세요.')};
 const step=(label:string,description:string)=><ListItem key={label} label={t(label)} description={<Text type="supporting">{t(description)}</Text>}/>;
 return <>
  <Banner status={state.s} title={state.t} description={state.d}/>
  <Text type="supporting" aria-live="polite">{status.message}</Text>
  <VStack gap={4} style={panel}>
   <Heading level={3}>{t('블루투스 제스처')}</Heading>
   <HStack gap={3}>
    <Button label={busy?t('진행 중…'):t('블루투스 제스처 켜기')} isDisabled={busy||!onBT||native} onClick={()=>send('bt-apply')}/>
    <Button label={t('원래대로 되돌리기')} variant="secondary" isDisabled={busy||(onBT&&!native)} onClick={()=>send('bt-restore')}/>
   </HStack>
   <Text type="supporting">{t('관리자 암호를 한 번 묻습니다. 적용하는 동안 블루투스 키보드·마우스·헤드폰이 몇 초 끊겼다가 다시 연결됩니다.')}</Text>
  </VStack>
  <VStack gap={4} style={panel}>
   <HStack gap={4} vAlign="center"><StackItem style={{flex:1}}><Heading level={3}>{t('설정 미러')}</Heading></StackItem><Switch label={t('설정 미러')} isLabelHidden value={!!bt?.mirror} onChange={v=>send('set-mirror',{value:v})}/></HStack>
   <Text>{t('블루투스 제스처가 동작하는 동안, 이 앱의 포인터·스크롤·제스처 설정을 macOS 트랙패드 설정에 그대로 씁니다. 앱 하나로 USB와 블루투스를 모두 조절할 수 있습니다.')}</Text>
   <Banner status="warning" title={t('macOS 설정은 모든 트랙패드에 공통입니다')} description={t('켜는 순간 앱 설정값이 macOS에 덮어써지며, MacBook 내장 트랙패드에도 적용됩니다(예: 포인터 속도, 회전, 관성 스크롤). 처음 켤 때 기존 macOS 값을 백업하므로 언제든 되돌릴 수 있습니다.')}/>
   <HStack gap={3}><Button label={t('macOS 설정 원래대로')} variant="secondary" isDisabled={!bt?.mirrorBackup} onClick={()=>send('mirror-restore')}/></HStack>
  </VStack>
  <Heading level={3}>{t('백업 위치')}</Heading>
  <List density="spacious">
   <ListItem label={t('블루투스 기록 원본')} description={<Text type="code">{bt?.cacheBackup??'—'}</Text>} endContent={<Text type="supporting">{t('관리자 전용')}</Text>}/>
   <ListItem label={t('macOS 트랙패드 설정')} description={bt?.mirrorBackup?<Text type="code">{bt.mirrorBackup}</Text>:t('설정 미러를 처음 켤 때 만들어집니다')} endContent={bt?.mirrorBackup?<Button label={t('Finder에서 보기')} variant="secondary" onClick={()=>send('reveal',{path:bt.mirrorBackup})}/>:undefined}/>
  </List>
  <Heading level={3}>{t('동작 원리')}</Heading>
  <List density="spacious">
   {step('1. 왜 안 됐나','macOS 14에는 USB-C 모델(제품 ID 0x0324)용 블루투스 드라이버가 없어 일반 마우스로 인식합니다. 터치 데이터는 앱에 도착하기 전에 버려집니다.')}
   {step('2. 무엇을 바꾸나','Mac의 블루투스 기록에 저장된 이 트랙패드의 제품 ID만 Lightning 모델(0x0265)로 바꿉니다. 트랙패드 펌웨어, macOS 시스템 파일, SIP 같은 보안 설정, 페어링 키는 바꾸지 않습니다.')}
   {step('3. 왜 동작하나','두 모델은 블루투스 통신 방식이 같아서, macOS에 이미 들어 있는 Magic Trackpad 2 드라이버가 그대로 동작합니다.')}
   {step('4. 적용 방식','기록을 바꾼 뒤 블루투스 서비스를 다시 시작해 macOS가 새 기록을 읽게 합니다. 기록 형식이 예상과 다르면 아무것도 바꾸지 않고 멈춥니다.')}
  </List>
  <Heading level={3}>{t('알아둘 점')}</Heading>
  <List density="spacious">
   {step('케이블로 Mac에 연결하면 풀립니다','다시 페어링되며 원래 ID로 돌아갑니다. 충전은 일반 USB-C 충전기를 쓰거나, 충전 후 이 화면에서 다시 켜세요.')}
   {step('시스템 정보 표시','시스템 정보에는 계속 0x0324로 보일 수 있습니다. 동작에는 영향이 없습니다.')}
   {step('출처','아이디어: GianMarioBen/MagicTrackPadCatalina (Catalina용). Trackpad Anything은 Sonoma 기록 형식에 맞춰 독립적으로 구현했습니다.')}
  </List>
 </>;
}

const gestureNote:Record<string,string>={lookup:' · 세 손가락 드래그와 선택 사용, ⌃⌘D 방식',pages:' · 켜면 두 손가락 좌우 스크롤 대신 페이지 이동',notifications:' · 메뉴 막대 시계 접근 방식',pressure:' · 압력 단계·햅틱 제어 검증 필요',clickForce:' · 압력 단계·햅틱 제어 검증 필요'};

export default function App(){const [page,setPage]=useState<string>(()=>{const h=decodeURIComponent(location.hash.slice(1));return nav.some(([l])=>l===h)?h:'포인터 및 스크롤';});const [status,setStatus]=useState<BridgeStatus>({running:false,usb:false,message:'',native:false,settings:defaults,engineActive:false,activating:false,trusted:false});
 useEffect(()=>{window.tcReceive=setStatus;send('status');const timer=setInterval(()=>send('status'),2000);if(!window.webkit)setStatus(s=>({...s,message:t('브라우저 미리보기 · 기기 제어 없음')}));return()=>{clearInterval(timer);delete window.tcReceive}},[]);
 setLang(status.lang);
 useEffect(()=>{document.documentElement.lang=status.lang==='zh'?'zh-Hans':status.lang||'ko';},[status.lang]);
 const available=status.native&&status.running;
 const btNative=status.transport==='Bluetooth'&&!!status.bluetooth?.native;
 const update=(key:string,value:number)=>{send('set-option',{key,value});};
 const toggle=(key:string,label:string)=><Switch label={label} isLabelHidden value={!!status.settings[key]} isDisabled={!status.native} onChange={v=>update(key,v?1:0)}/>;
 const pressure=(id:string)=>id==='pressure'||id==='clickForce';
 const gestureRows=(group?:string)=>features.filter(f=>!group||f.group===group).map(f=><ListItem key={f.id} label={t(f.name)} description={t(f.gesture)+(gestureNote[f.id]?t(gestureNote[f.id]):'')} endContent={featureOption[f.id]?toggle(featureOption[f.id],t(f.name)):<Text type="supporting">{pressure(f.id)?t('검증 중'):t('항상 사용')}</Text>}/>);
 const quickHint=btNative?(status.bluetooth?.mirror?t('블루투스 연결 중 · 설정 미러로 macOS 트랙패드 설정에 바로 반영됩니다'):t('블루투스 연결 중 · 이 값은 USB 연결에만 적용됩니다. 블루투스에도 쓰려면 블루투스 연결에서 설정 미러를 켜세요')):status.engineActive?t('변경한 값은 다음 손가락 동작부터 적용됩니다'):t('설정 적용 시작 후 입력에 반영됩니다');
 const quickSettings=<VStack gap={4} style={panel}><HStack gap={4}><StackItem style={{flex:1}}><Heading level={3}>{t('속도 및 클릭')}</Heading></StackItem><Button label={t('기본값 복원')} variant="secondary" isDisabled={!status.native} onClick={()=>send('reset-options')}/></HStack><HStack gap={6} vAlign="start"><StackItem style={{flex:1,minWidth:0}}><Slider label={t('포인터 속도')} value={status.settings.pointerSpeed} min={.05} max={1} step={.01} valueDisplay="text" formatValue={v=>`${Math.round(v/.3*100)}%`} isDisabled={!status.native} onChange={(v:number)=>update('pointerSpeed',v)}/></StackItem><StackItem style={{flex:1,minWidth:0}}><Slider label={t('스크롤 속도')} value={status.settings.scrollSpeed} min={.02} max={1} step={.01} valueDisplay="text" formatValue={v=>`${Math.round(v/.22*100)}%`} isDisabled={!status.native} onChange={(v:number)=>update('scrollSpeed',v)}/></StackItem></HStack><HStack gap={6}><Switch label={t('탭하여 클릭')} value={!!status.settings.tapToClick} isDisabled={!status.native} onChange={v=>update('tapToClick',+v)}/><Switch label={t('자연스러운 스크롤')} value={!!status.settings.naturalScroll} isDisabled={!status.native} onChange={v=>update('naturalScroll',+v)}/></HStack><Text type="supporting">{quickHint}</Text></VStack>;
 const activation=!status.engineActive&&status.native&&!btNative?<HStack gap={4} vAlign="center"><StackItem style={{flex:1}}><Text type="label">{status.running?t('기존 입력 앱 사용 중'):t('입력 제어 시작 전')}</Text><Text type="supporting">{t('설정을 적용하려면 새 입력 엔진으로 전환하세요. 기존 앱은 복구용으로 보관됩니다.')}</Text></StackItem><Button label={status.activating?t('연결 중…'):t('설정 적용 시작')} isDisabled={status.activating} onClick={()=>send('activate')}/></HStack>:null;
 const connected=status.deviceConnected||status.usb;
 const connection=connected?t('연결됨 · {0}',status.transport||'USB')+(status.transport==='Bluetooth'?(status.bluetooth?.native?t(' · 제스처'):t(' · 제스처 꺼짐')):''):t('연결되지 않음');
 const btDiag=status.transport==='Bluetooth'?(status.bluetooth?.native?t('연결됨 · macOS 기본 드라이버로 제스처 동작'):t('연결됨 · 제스처 꺼짐 (블루투스 연결 메뉴에서 켜기)')):t('미연결');
 const sidebar=<LayoutPanel hasDivider padding={0}><VStack gap={6} style={{paddingBlock:'var(--spacing-6)',paddingInline:'var(--spacing-3)',minWidth:'14rem'}}>
  <picture style={{alignSelf:'center'}}><source srcSet="assets/logo-dark.png" media="(prefers-color-scheme: dark)"/><img src="assets/logo-light.png" alt="Trackpad Anything" style={{width:'11rem',display:'block'}}/></picture>
  <VStack gap={2} style={{padding:'var(--spacing-4)'}}><img src="assets/trackpad.png" alt="" style={{width:'8rem',alignSelf:'center'}}/><Text type="label">Magic Trackpad</Text><HStack gap={2} vAlign="center"><svg viewBox="0 0 8 8" aria-hidden="true" style={{width:'0.5rem',height:'0.5rem',flexShrink:0}}><circle cx="4" cy="4" r="4" fill={connected?'var(--color-success)':'var(--color-text-secondary)'}/></svg><Text type="supporting">{connection}</Text>{!!connected&&<HStack gap={1} vAlign="center"><svg viewBox="0 0 20 12" aria-hidden="true" style={{width:'1.25rem',height:'0.75rem',color:muted}}><rect x="1" y="1" width="15" height="10" rx="2" fill="none" stroke="currentColor"/><path d="M18 4v4" stroke="currentColor" strokeWidth="2"/>{status.batteryPercent!=null&&<rect x="3" y="3" width={11*status.batteryPercent/100} height="6" rx="1" fill="currentColor"/>}</svg><Text type="supporting" aria-label={status.batteryPercent!=null?t('배터리 {0}%',status.batteryPercent):t('배터리 잔량 확인 중')}>{status.batteryPercent!=null?`${status.batteryPercent}%`:'—'}</Text></HStack>}</HStack></VStack>
  <Divider/>
  <List density="spacious">{nav.map(([key,icon])=><ListItem key={key} label={t(key)} startContent={<Icon icon={icon}/>} isSelected={page===key} onClick={()=>setPage(key)}/>)}</List>
  <VStack gap={3} style={{padding:'var(--spacing-4)',marginTop:'auto'}}>
   {/* Native popup menu: Astryx Selector's popover needs CSS anchor positioning, which macOS 14's WebKit lacks. */}
   <select aria-label={t('언어')} value={status.languagePref??'system'} onChange={e=>send('set-language',{value:e.target.value})} style={{font:'inherit',fontSize:'var(--font-size-sm)',color:'var(--color-text-primary)',background:'var(--color-background-surface)',border:'1px solid var(--color-border)',borderRadius:'var(--radius-control)',padding:'var(--spacing-1) var(--spacing-2)',alignSelf:'flex-start'}}>
    <option value="system">{t('시스템 언어')}</option>
    {languages.map(l=><option key={l.value} value={l.value}>{l.label}</option>)}
   </select>
   <VStack gap={1}><Text type="label">Trackpad Anything {status.version??'1.0.0'}</Text><Text type="supporting" weight="semibold">KUGNUS LAB</Text></VStack>
  </VStack>
 </VStack></LayoutPanel>;
 return <Layout height="fill" contentWidth={1200} style={{minHeight:'100dvh'}} start={sidebar} content={<LayoutContent padding={4}><VStack gap={6} style={{padding:'var(--spacing-4)'}}><Heading level={1} weight="semibold" color="inherit" style={{fontSize:'var(--font-size-3xl)'}}>{t(page)}</Heading>
 {page==='화면 전환'?<>{activation}<Text type="supporting" aria-live="polite">{status.message}</Text><Divider/><HStack gap={6} vAlign="start"><StackItem style={{flex:1,minWidth:0}}><SpaceDiagram/></StackItem><VStack gap={5} style={{...panel,width:'16rem',flexShrink:0,minHeight:'27rem'}}><Heading level={3}>{t('이 Mac에서 전환')}</Heading><Switch label={t('네 손가락 좌우 쓸기')} value={!!status.settings.spaces} isDisabled={!status.native} onChange={v=>update("spaces",+v)}/><Switch label={t('위로 쓸어 Mission Control')} value={!!status.settings.missionControl} isDisabled={!status.native} onChange={v=>update("missionControl",+v)}/><Divider/><Text type="supporting">{t('키보드 단축키')}</Text><Keys/><Divider/><Button label={t('이전 데스크탑')} variant="secondary" isDisabled={!available} onClick={()=>send('left')}/><Button label={t('다음 데스크탑')} variant="secondary" isDisabled={!available} onClick={()=>send('right')}/><Button label={t('Mission Control 열기')} isDisabled={!status.native} onClick={()=>send('mission')}/><Text type="supporting">{t('Parsec과 Jump Desktop의 전체 화면에서도 로컬 데스크탑을 전환합니다')}</Text></VStack></HStack><Divider/><Heading level={3}>{t('현재 동작')}</Heading><List density="spacious">{features.filter(f=>['spaces','mission','drag'].includes(f.id)).map(f=><ListItem key={f.id} label={t(f.name)} endContent={<Text color="secondary">{t(f.gesture)}</Text>}/>)}</List></>:
 page==='제스처'?<>{activation}<Text color="secondary">{t('제스처를 켜거나 끄세요. 새로 추가된 동작은 처음에는 꺼져 있습니다')}</Text>{groups.map(group=><VStack key={group} gap={3}><Heading level={3}>{t(group)}</Heading><List density="spacious">{gestureRows(group)}</List></VStack>)}</>:
 page==='포인터 및 스크롤'?<>{quickSettings}{activation}<List density="spacious">{gestureRows('포인터 및 클릭')}{gestureRows('스크롤 및 확대')}</List><Text type="supporting">{t('찾아보기와 세 손가락 드래그는 동시에 사용할 수 없습니다. 하나를 켜면 다른 기능은 꺼집니다.')}</Text></>:
 page==='블루투스 연결'?<BluetoothPage status={status}/>:
 page==='단축키'?<>{!status.trusted&&<Banner status="warning" title={t('손쉬운 사용 권한이 필요합니다')} description={t('시스템 설정 > 개인정보 보호 및 보안 > 손쉬운 사용에서 Trackpad Anything을 허용하세요.')}/>}<Heading level={3}>{t('로컬 데스크탑 전환')}</Heading><List density="spacious"><ListItem label={t('키보드 단축키 사용')} description={<Text type="supporting">{t('Karabiner 없이 동작합니다. 원격 앱 전체 화면에서도 이 Mac의 데스크탑을 전환합니다.')}</Text>} endContent={<Switch label={t('키보드 단축키 사용')} isLabelHidden value={!!status.hotkeys} onChange={v=>send('set-hotkeys',{value:v})}/>}/><ListItem label={t('이전 데스크탑')} endContent={<Text type="code">⌃ ⌥ ⌘ ←</Text>}/><ListItem label={t('다음 데스크탑')} endContent={<Text type="code">⌃ ⌥ ⌘ →</Text>}/></List><Text color="secondary">{t('단축키 변경은 다음 버전에서 제공할 예정입니다.')}</Text></>:
 <>{activation}<Text>{status.message}</Text><List density="spacious"><ListItem label={t('입력 엔진')} description={status.engineActive?t('설정 가능한 입력 엔진 실행 중'):status.running?t('이전 앱(0.2) 실행 중'):t('연결 대기')}/><ListItem label={t('USB 전송')} description={status.usb?t('USB 연결됨'):t('미연결')}/><ListItem label="Bluetooth" description={btDiag}/><ListItem label={t('로그인 시 자동 실행')} description={t('Mac에 로그인하면 메뉴 막대에서 바로 시작합니다')} endContent={<Switch label={t('로그인 시 자동 실행')} isLabelHidden value={!!status.loginItem} onChange={v=>send('set-login',{value:v})}/>}/></List><Button label={t('상태 새로고침')} variant="secondary" onClick={()=>send('status')}/><Text type="supporting">{t('수신 프레임: {0} · 최근 동작: {1}',status.engine?.frames??0,status.engine?.action??t('대기'))}</Text><HStack gap={3}><Button label={t('USB 다시 연결')} variant="secondary" isDisabled={!status.engineActive} onClick={()=>send("reconnect")}/><Button label={t('이전 앱(0.2)으로 복구')} variant="secondary" isDisabled={!status.engineActive} onClick={()=>send("restore")}/></HStack><Text type="supporting">{t('입력 중 창을 닫아도 메뉴 막대에서 계속 동작합니다.')}</Text></>}
 </VStack></LayoutContent>}/>}
