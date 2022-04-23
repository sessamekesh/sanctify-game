/* eslint-disable */
import Long from "long";
import * as _m0 from "protobufjs/minimal";

export const protobufPackage = "sanctify.pve.pb";

export interface NetworkSimulationParameters {
  /** Client -> server latency characteristics */
  clientServerLatencyMedian: number;
  clientServerLatencyStdev: number;
  clientServerPacketLossPct: number;
  /** Server -> client latency characteristics */
  serverClientLatencyMedian: number;
  serverClientLatencyStdev: number;
  serverClientPacketLossPct: number;
}

export interface RenderSettings {
  swapChainResizeLatency: number;
}

export interface DebugSettings {
  frameTime: number;
}

export interface PveOfflineClientConfig {
  initialNetworkSimState: NetworkSimulationParameters | undefined;
  renderSettings: RenderSettings | undefined;
  debugSettings: DebugSettings | undefined;
}

function createBaseNetworkSimulationParameters(): NetworkSimulationParameters {
  return {
    clientServerLatencyMedian: 0,
    clientServerLatencyStdev: 0,
    clientServerPacketLossPct: 0,
    serverClientLatencyMedian: 0,
    serverClientLatencyStdev: 0,
    serverClientPacketLossPct: 0,
  };
}

export const NetworkSimulationParameters = {
  encode(
    message: NetworkSimulationParameters,
    writer: _m0.Writer = _m0.Writer.create()
  ): _m0.Writer {
    if (message.clientServerLatencyMedian !== 0) {
      writer.uint32(13).float(message.clientServerLatencyMedian);
    }
    if (message.clientServerLatencyStdev !== 0) {
      writer.uint32(21).float(message.clientServerLatencyStdev);
    }
    if (message.clientServerPacketLossPct !== 0) {
      writer.uint32(29).float(message.clientServerPacketLossPct);
    }
    if (message.serverClientLatencyMedian !== 0) {
      writer.uint32(37).float(message.serverClientLatencyMedian);
    }
    if (message.serverClientLatencyStdev !== 0) {
      writer.uint32(45).float(message.serverClientLatencyStdev);
    }
    if (message.serverClientPacketLossPct !== 0) {
      writer.uint32(53).float(message.serverClientPacketLossPct);
    }
    return writer;
  },

  decode(
    input: _m0.Reader | Uint8Array,
    length?: number
  ): NetworkSimulationParameters {
    const reader = input instanceof _m0.Reader ? input : new _m0.Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseNetworkSimulationParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.clientServerLatencyMedian = reader.float();
          break;
        case 2:
          message.clientServerLatencyStdev = reader.float();
          break;
        case 3:
          message.clientServerPacketLossPct = reader.float();
          break;
        case 4:
          message.serverClientLatencyMedian = reader.float();
          break;
        case 5:
          message.serverClientLatencyStdev = reader.float();
          break;
        case 6:
          message.serverClientPacketLossPct = reader.float();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): NetworkSimulationParameters {
    return {
      clientServerLatencyMedian: isSet(object.clientServerLatencyMedian)
        ? Number(object.clientServerLatencyMedian)
        : 0,
      clientServerLatencyStdev: isSet(object.clientServerLatencyStdev)
        ? Number(object.clientServerLatencyStdev)
        : 0,
      clientServerPacketLossPct: isSet(object.clientServerPacketLossPct)
        ? Number(object.clientServerPacketLossPct)
        : 0,
      serverClientLatencyMedian: isSet(object.serverClientLatencyMedian)
        ? Number(object.serverClientLatencyMedian)
        : 0,
      serverClientLatencyStdev: isSet(object.serverClientLatencyStdev)
        ? Number(object.serverClientLatencyStdev)
        : 0,
      serverClientPacketLossPct: isSet(object.serverClientPacketLossPct)
        ? Number(object.serverClientPacketLossPct)
        : 0,
    };
  },

  toJSON(message: NetworkSimulationParameters): unknown {
    const obj: any = {};
    message.clientServerLatencyMedian !== undefined &&
      (obj.clientServerLatencyMedian = message.clientServerLatencyMedian);
    message.clientServerLatencyStdev !== undefined &&
      (obj.clientServerLatencyStdev = message.clientServerLatencyStdev);
    message.clientServerPacketLossPct !== undefined &&
      (obj.clientServerPacketLossPct = message.clientServerPacketLossPct);
    message.serverClientLatencyMedian !== undefined &&
      (obj.serverClientLatencyMedian = message.serverClientLatencyMedian);
    message.serverClientLatencyStdev !== undefined &&
      (obj.serverClientLatencyStdev = message.serverClientLatencyStdev);
    message.serverClientPacketLossPct !== undefined &&
      (obj.serverClientPacketLossPct = message.serverClientPacketLossPct);
    return obj;
  },

  fromPartial<I extends Exact<DeepPartial<NetworkSimulationParameters>, I>>(
    object: I
  ): NetworkSimulationParameters {
    const message = createBaseNetworkSimulationParameters();
    message.clientServerLatencyMedian = object.clientServerLatencyMedian ?? 0;
    message.clientServerLatencyStdev = object.clientServerLatencyStdev ?? 0;
    message.clientServerPacketLossPct = object.clientServerPacketLossPct ?? 0;
    message.serverClientLatencyMedian = object.serverClientLatencyMedian ?? 0;
    message.serverClientLatencyStdev = object.serverClientLatencyStdev ?? 0;
    message.serverClientPacketLossPct = object.serverClientPacketLossPct ?? 0;
    return message;
  },
};

function createBaseRenderSettings(): RenderSettings {
  return { swapChainResizeLatency: 0 };
}

export const RenderSettings = {
  encode(
    message: RenderSettings,
    writer: _m0.Writer = _m0.Writer.create()
  ): _m0.Writer {
    if (message.swapChainResizeLatency !== 0) {
      writer.uint32(13).float(message.swapChainResizeLatency);
    }
    return writer;
  },

  decode(input: _m0.Reader | Uint8Array, length?: number): RenderSettings {
    const reader = input instanceof _m0.Reader ? input : new _m0.Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseRenderSettings();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.swapChainResizeLatency = reader.float();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): RenderSettings {
    return {
      swapChainResizeLatency: isSet(object.swapChainResizeLatency)
        ? Number(object.swapChainResizeLatency)
        : 0,
    };
  },

  toJSON(message: RenderSettings): unknown {
    const obj: any = {};
    message.swapChainResizeLatency !== undefined &&
      (obj.swapChainResizeLatency = message.swapChainResizeLatency);
    return obj;
  },

  fromPartial<I extends Exact<DeepPartial<RenderSettings>, I>>(
    object: I
  ): RenderSettings {
    const message = createBaseRenderSettings();
    message.swapChainResizeLatency = object.swapChainResizeLatency ?? 0;
    return message;
  },
};

function createBaseDebugSettings(): DebugSettings {
  return { frameTime: 0 };
}

export const DebugSettings = {
  encode(
    message: DebugSettings,
    writer: _m0.Writer = _m0.Writer.create()
  ): _m0.Writer {
    if (message.frameTime !== 0) {
      writer.uint32(13).float(message.frameTime);
    }
    return writer;
  },

  decode(input: _m0.Reader | Uint8Array, length?: number): DebugSettings {
    const reader = input instanceof _m0.Reader ? input : new _m0.Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseDebugSettings();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.frameTime = reader.float();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): DebugSettings {
    return {
      frameTime: isSet(object.frameTime) ? Number(object.frameTime) : 0,
    };
  },

  toJSON(message: DebugSettings): unknown {
    const obj: any = {};
    message.frameTime !== undefined && (obj.frameTime = message.frameTime);
    return obj;
  },

  fromPartial<I extends Exact<DeepPartial<DebugSettings>, I>>(
    object: I
  ): DebugSettings {
    const message = createBaseDebugSettings();
    message.frameTime = object.frameTime ?? 0;
    return message;
  },
};

function createBasePveOfflineClientConfig(): PveOfflineClientConfig {
  return {
    initialNetworkSimState: undefined,
    renderSettings: undefined,
    debugSettings: undefined,
  };
}

export const PveOfflineClientConfig = {
  encode(
    message: PveOfflineClientConfig,
    writer: _m0.Writer = _m0.Writer.create()
  ): _m0.Writer {
    if (message.initialNetworkSimState !== undefined) {
      NetworkSimulationParameters.encode(
        message.initialNetworkSimState,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.renderSettings !== undefined) {
      RenderSettings.encode(
        message.renderSettings,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.debugSettings !== undefined) {
      DebugSettings.encode(
        message.debugSettings,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: _m0.Reader | Uint8Array,
    length?: number
  ): PveOfflineClientConfig {
    const reader = input instanceof _m0.Reader ? input : new _m0.Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBasePveOfflineClientConfig();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.initialNetworkSimState = NetworkSimulationParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.renderSettings = RenderSettings.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.debugSettings = DebugSettings.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): PveOfflineClientConfig {
    return {
      initialNetworkSimState: isSet(object.initialNetworkSimState)
        ? NetworkSimulationParameters.fromJSON(object.initialNetworkSimState)
        : undefined,
      renderSettings: isSet(object.renderSettings)
        ? RenderSettings.fromJSON(object.renderSettings)
        : undefined,
      debugSettings: isSet(object.debugSettings)
        ? DebugSettings.fromJSON(object.debugSettings)
        : undefined,
    };
  },

  toJSON(message: PveOfflineClientConfig): unknown {
    const obj: any = {};
    message.initialNetworkSimState !== undefined &&
      (obj.initialNetworkSimState = message.initialNetworkSimState
        ? NetworkSimulationParameters.toJSON(message.initialNetworkSimState)
        : undefined);
    message.renderSettings !== undefined &&
      (obj.renderSettings = message.renderSettings
        ? RenderSettings.toJSON(message.renderSettings)
        : undefined);
    message.debugSettings !== undefined &&
      (obj.debugSettings = message.debugSettings
        ? DebugSettings.toJSON(message.debugSettings)
        : undefined);
    return obj;
  },

  fromPartial<I extends Exact<DeepPartial<PveOfflineClientConfig>, I>>(
    object: I
  ): PveOfflineClientConfig {
    const message = createBasePveOfflineClientConfig();
    message.initialNetworkSimState =
      object.initialNetworkSimState !== undefined &&
      object.initialNetworkSimState !== null
        ? NetworkSimulationParameters.fromPartial(object.initialNetworkSimState)
        : undefined;
    message.renderSettings =
      object.renderSettings !== undefined && object.renderSettings !== null
        ? RenderSettings.fromPartial(object.renderSettings)
        : undefined;
    message.debugSettings =
      object.debugSettings !== undefined && object.debugSettings !== null
        ? DebugSettings.fromPartial(object.debugSettings)
        : undefined;
    return message;
  },
};

type Builtin =
  | Date
  | Function
  | Uint8Array
  | string
  | number
  | boolean
  | undefined;

export type DeepPartial<T> = T extends Builtin
  ? T
  : T extends Array<infer U>
  ? Array<DeepPartial<U>>
  : T extends ReadonlyArray<infer U>
  ? ReadonlyArray<DeepPartial<U>>
  : T extends {}
  ? { [K in keyof T]?: DeepPartial<T[K]> }
  : Partial<T>;

type KeysOfUnion<T> = T extends T ? keyof T : never;
export type Exact<P, I extends P> = P extends Builtin
  ? P
  : P & { [K in keyof P]: Exact<P[K], I[K]> } & Record<
        Exclude<keyof I, KeysOfUnion<P>>,
        never
      >;

if (_m0.util.Long !== Long) {
  _m0.util.Long = Long as any;
  _m0.configure();
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
