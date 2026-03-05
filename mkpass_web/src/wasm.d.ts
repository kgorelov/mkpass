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
        push_back(value: number): void;
        delete(): void;
    };
    MkPass(ctx: {
        password: string;
        service: string;
        char_classes: any;
        algorithm: number;
        length: number;
        custom_chars: string;
    }): string;
}

declare global {
    interface Window {
        mkpass_wasm: (options?: any) => Promise<MkPassModule>;
    }
}
