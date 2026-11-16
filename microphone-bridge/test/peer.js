let peer;
const samples=[];
window.answer=async sdp=>{
  peer ||= new RTCPeerConnection({encodedInsertableStreams:true});
  peer.ontrack=event=>{
    if(event.track.kind!=='audio')return;
    const decoder=new AudioDecoder({output:data=>{
      const pcm=new Float32Array(data.numberOfFrames);
      data.copyTo(pcm,{planeIndex:0,format:'f32-planar'});
      samples.push({at:performance.now(),rms:Math.sqrt(pcm.reduce((sum,v)=>sum+v*v,0)/pcm.length)});
      while(samples.length>200)samples.shift();
      data.close();
    },error:error=>console.error('decode',error.message)});
    decoder.configure({codec:'opus',sampleRate:48000,numberOfChannels:2});
    const streams=event.receiver.createEncodedStreams();let stamp=0;
    streams.readable.pipeThrough(new TransformStream({transform(frame,controller){
      decoder.decode(new EncodedAudioChunk({type:'key',timestamp:stamp,data:frame.data}));stamp+=20000;
      controller.enqueue(frame);
    }})).pipeTo(streams.writable).catch(()=>{});
  };
  peer.ondatachannel = event => { event.channel.onmessage = () => {}; };
  await peer.setRemoteDescription({ type: 'offer', sdp });
  await peer.setLocalDescription(await peer.createAnswer());
  await Promise.race([new Promise(resolve => {
    if (peer.iceGatheringState === 'complete') return resolve();
    peer.addEventListener('icegatheringstatechange', () => { if (peer.iceGatheringState === 'complete') resolve(); });
  }), new Promise(resolve => setTimeout(resolve, 2500))]);
  return { sdp: peer.localDescription.sdp };
};
window.candidates = async values => {
  for (const c of values) await peer.addIceCandidate(c);
  return [];
};
window.packetCount = async () => {
  let result = 0;
  (await peer.getStats()).forEach(s => { if (s.type === 'inbound-rtp' && s.kind === 'audio') result += s.packetsReceived || 0; });
  return result;
};
window.audioLevel=async()=>{
  const since=performance.now();await new Promise(resolve=>setTimeout(resolve,750));
  return Math.max(0,...samples.filter(s=>s.at>=since).map(s=>s.rms));
};
window.diagnostics=async()=>({samples:samples.slice(-3),stats:[...(await peer.getStats()).values()].filter(s=>s.type==='inbound-rtp'&&s.kind==='audio')});
