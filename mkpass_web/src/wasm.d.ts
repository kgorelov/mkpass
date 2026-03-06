export interface MkPassModule {
    Algorithm: {
        Argon2: number;
        SlowSha512: number;
        Old: number;
    };
    CharacterClass: {
        LOWERCASE: number;
        UPPERCASE: number;
        DIGITS: number;
        SYMBOLS: number;
        CUSTOM: number;
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
