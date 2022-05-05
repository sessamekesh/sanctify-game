import { SanctifyPveOfflineClientInstance, SanctifyPveOfflineClientLoadService } from "@sanctify/pve-offline-wasm-wrapper";
import Head from "next/head";
import Script from "next/script";
import React, { useEffect, useRef, useState } from "react";
import SanctifyGameApi from '../services/pve-offline-load-service';

type GameplayElementProps = {
  api: SanctifyPveOfflineClientLoadService,
  gameClient: SanctifyPveOfflineClientInstance|null,
  setGameClient: React.Dispatch<React.SetStateAction<SanctifyPveOfflineClientInstance|null>>,
};

function GameplayElement({api, gameClient, setGameClient}: GameplayElementProps) {
  const attachDiv = (div: HTMLDivElement) => {
    const canvas = api.getGameCanvas();
    const currentParent = canvas.parentElement;

    if (currentParent === div) {
      return;
    }

    if (div == null) {
      if (gameClient) {
        gameClient.pause();
      }
      return;
    }

    div.appendChild(canvas);
    api.getGameClient().then(client => {
      client.start();
      setGameClient(client);
    });
  };

  return <div ref={attachDiv} style={{
    position: 'absolute',
    left: 0,
    top: 0,
    right: 0,
    bottom: 0,
  }} />;
}

export default function QuickPlay() {
  const api = SanctifyGameApi('');

  const [scriptLoaded, setScriptLoaded] = useState(false);
  const [gameClient, setGameClient] = useState<SanctifyPveOfflineClientInstance|null>(null);

  return (
    <div>
      <Head>
        <title>Loading...</title>
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <Script src={api.gameScriptUrl()} strategy="afterInteractive" onLoad={()=>setScriptLoaded(true)} />
      {!scriptLoaded && <div>Loading script...</div>}
      {!gameClient && <div>Loading game...</div>}
      {scriptLoaded && <GameplayElement api={api} gameClient={gameClient} setGameClient={setGameClient} />}
    </div>
  )
}
