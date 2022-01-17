import React from 'react';
import { SanctifyGameClientLoadService, SanctifyGameClientInstance } from '@indigo/sanctify-game-wasm-bridge';

interface Props {
  baseUrl: string;
}

interface State {
  wgpuError: boolean,
}

export class DevClientApp extends React.Component<Props, State> {
  private client: SanctifyGameClientInstance|null = null;
  private loadService: SanctifyGameClientLoadService;

  constructor(props: Props) {
    super(props);

    this.loadService = new SanctifyGameClientLoadService(props.baseUrl);

    this.state = {
      wgpuError: false,
    };
  }

  async componentDidMount() {
    const canvas = this.loadService.getGameCanvas();
    canvas.style.backgroundColor = 'lightred';
    canvas.style.position = 'absolute';
    canvas.style.left = '0';
    canvas.style.right = '0';
    canvas.style.top = '0';
    canvas.style.bottom = '0';

    try {
      this.client = await this.loadService.getGameClient();
      this.client.start();
    } catch (e: any) {
      if (e['message'] === 'WebGPU is not supported') {
        this.setState({wgpuError: true});
      } else {
        throw e;
      }
    }
  }

  componentWillUnmount() {
    if (this.client) {
      this.client.destroy();
    }
  }

  render() {
    return <div className="App">{this.innerState()}</div>;
  }

  private renderCanvasParent = (element: HTMLDivElement) => {
    if (element) {
      element.prepend(this.loadService.getGameCanvas());
    }
  }

  private innerState() {
    if (this.state.wgpuError) {
      return (
        <div style={{position: 'absolute', margin: 0, padding: 0, left: 0, right: 0, top: 0, bottom: 0}}>
          <p>WebGPu is not supported by this browser</p>
        </div>
      );
    }

    return (
      <div style={{position: 'absolute', margin: 0, padding: 0, left: 0, right: 0, top: 0, bottom: 0}}>
        <div style={this.wrapperDivStyle()} ref={this.renderCanvasParent}></div>
      </div>
    );
  }

  private wrapperDivStyle(): React.CSSProperties {
    const props: React.CSSProperties = {
      margin: 0,
      padding: 0,
      position: 'absolute',
      width: '100vw',
      height: '100vh'
    };

    return props;
  }
}