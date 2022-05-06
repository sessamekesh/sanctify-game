import React from "react";
import styles from './overlay.module.scss';

type Props = {
  leftOffset: number,
  topOffset: number,
};

export default function BurgerMenuOverlay({children, leftOffset, topOffset}: React.PropsWithChildren<Props>) {
  return (
    <div className={styles['overlay-root']} style={{left: leftOffset, top: topOffset}}>
      {children}
    </div>
  );
}
