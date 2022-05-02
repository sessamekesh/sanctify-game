import React, { useRef } from 'react';
import BurgerMenuIcon from './icon';
import Overlay from './overlay';

type Props = {};

export default function BurgerMenu({children}: React.PropsWithChildren<Props>) {
  const [isMenuOpen, setIsMenuOpen] = React.useState(false);
  const parentRef = useRef<HTMLDivElement>(null);

  const left = parentRef.current?.parentElement?.getBoundingClientRect().left || 0;
  const bottom = parentRef.current?.parentElement?.getBoundingClientRect().bottom || 0;

  return (
    <div ref={parentRef}>
      <BurgerMenuIcon onOpen={()=>{setIsMenuOpen(!isMenuOpen)}} />
      {isMenuOpen && (
        <Overlay leftOffset={left} topOffset={bottom}>{children}</Overlay>
      )}
    </div>
  );
}

