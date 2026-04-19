export interface EmbindEnum {
    value: number;
}

export interface QrCodeData {
    size: number;
    data: {
        size(): number;
        get(index: number): boolean;
    };
}

export interface MkPassModule {
    Algorithm: {
        Argon2: EmbindEnum;
        SlowSha512: EmbindEnum;
        Old: EmbindEnum;
        Passphrase_Diceware_EFF_Large: EmbindEnum;
        Passphrase_Wordnet_Pattern: EmbindEnum;
    };
    PassphraseSeparator: {
        CamelCase: EmbindEnum;
        SnakeCase: EmbindEnum;
    };
    CharacterClass: {
        LOWERCASE: EmbindEnum;
        UPPERCASE: EmbindEnum;
        DIGITS: EmbindEnum;
        SYMBOLS: EmbindEnum;
        CUSTOM: EmbindEnum;
    };
    VectorCharacterClass: {
        new (): any;
        push_back(value: any): void;
        delete(): void;
    };
    VectorBool: {
        size(): number;
        get(index: number): boolean;
    };
    MkPass(
        password: string,
        service: string,
        char_classes: any,
        algorithm: number,
        length: number,
        custom_chars: string,
        separator: number
    ): string;
    GenerateQrCode(text: string): QrCodeData;
    getExceptionMessage(ptr: number): string;
}

declare global {
    interface Window {
        mkpass_wasm: (options?: any) => Promise<MkPassModule>;
    }
}
