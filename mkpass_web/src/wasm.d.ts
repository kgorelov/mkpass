export interface EmbindEnum {
    value: number;
}

export interface MkPassModule {
    Algorithm: {
        Argon2: EmbindEnum;
        SlowSha512: EmbindEnum;
        Old: EmbindEnum;
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
    MkPass(
        password: string,
        service: string,
        char_classes: any,
        algorithm: number,
        length: number,
        custom_chars: string
    ): string;
    getExceptionMessage(ptr: number): string;
}

declare global {
    interface Window {
        mkpass_wasm: (options?: any) => Promise<MkPassModule>;
    }
}
