// import {useEffect, useRef} from "react";
// import {Scene, type SceneConfig} from "@/features/stream/models/Scene.ts";
// import {useLiveStream} from "@/features/stream/hooks/useLiveStream.tsx";
// import {VideoCapture, ImageCapture} from "@/features/stream/models/Capture.ts";
//
// const sceneConfig: SceneConfig = {
//     resolution: {width: 1280, height: 720},
//     backgroundColor: '#8480c6',
//     videoCaptures: [
//         new VideoCapture(null, {
//                 x: 0, y: 360
//             }, {
//                 width: 1280, height: 360
//             },
//             '#ed5e5e'
//             , 0)
//     ],
//     imageCaptures: [
//         new ImageCapture(null, {
//                 x: 640, y: 0
//         }, {
//             width: 640, height: 360
//         }, '#ff02f3', 2)
//     ]
// }
// const LiveStream = () => {
//     const {loadScene, outputStream} = useLiveStream();
//     const videoRef = useRef<HTMLVideoElement>(null);
//     // ✅ Run once on mount only
//     useEffect(() => {
//         loadScene(Scene.create(sceneConfig));
//     }, [loadScene]);
//
//     // ✅ Only sync the stream to the video element when outputStream changes
//     useEffect(() => {
//         if (videoRef.current && outputStream) {
//             videoRef.current.srcObject = outputStream;
//         }
//     }, [outputStream]);
//     return (<div className="LiveStream">
//             <h1>Live Stream</h1>
//             <p>This is where the live stream will be displayed.</p>
//             <video ref={videoRef} autoPlay muted style={{width: '100%', height: 'auto'}}/>
//         </div>
//     );
// }
// export default LiveStream;