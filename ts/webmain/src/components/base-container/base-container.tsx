import React from "react";
import BaseHeader from './base-header';

import styles from './base-container.module.scss';

type Props = {
  children: React.ReactNode;
};

const BaseContainer = ({children, ...rest}: React.PropsWithChildren<Props>) => {
  return (
    <div className={styles['container']} {...rest}>
      <BaseHeader />
      {children}
    </div>
  );
};

export default BaseContainer;
