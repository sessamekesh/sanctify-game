import { AppProps } from 'next/app';
import '../styles/globals.scss';

export default function SanctifyWebBaseApp({Component, pageProps}: AppProps) {
  return <Component {...pageProps} />;
};
