import { NitroModules } from "react-native-nitro-modules";
import type { Adapter } from "./specs/Adapter.nitro";


export const HybridAdapter = NitroModules.createHybridObject<Adapter>("Adapter");