/*
 * Pony++ Debug Adapter (DAP)
 *
 * 实现 Debug Adapter Protocol:
 * - initialize, launch, setBreakpoints
 * - continue, next, stepIn, stepOut
 * - stackTrace, scopes, variables
 * - threads (每个Actor一个线程)
 *
 * 连接到 ponyppc --dap 模式
 */

import {
    DebugSession,
    InitializedEvent,
    StoppedEvent,
    ContinuedEvent,
    TerminatedEvent,
    Thread,
    StackFrame,
    Scope,
    Variable,
    Breakpoint,
    Source
} from '@vscode/debugadapter';

import { DebugProtocol } from '@vscode/debugprotocol';
import * as net from 'net';
import * as path from 'path';

interface PonyppLaunchArgs extends DebugProtocol.LaunchRequestArguments {
    program: string;
    args?: string[];
    stopOnEntry?: boolean;
}

export class PonyppDebugSession extends DebugSession {
    private _connection: net.Socket | null = null;
    private _breakpoints: Map<string, Breakpoint[]> = new Map();
    private _actorThreads: Map<number, string> = new Map();
    private _variableHandles: Map<number, any> = new Map();
    private _nextHandleId = 1;

    public constructor() {
        super();
        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerPathFormat('native');
    }

    protected initializeRequest(
        response: DebugProtocol.InitializeResponse
    ): void {
        response.body = {
            supportsConfigurationDoneRequest: true,
            supportsSetVariable: true,
            supportsRestartRequest: false,
            supportsStepBack: false,
            supportsEvaluateForHovers: false,
            supportsConditionalBreakpoints: true,
            supportsHitConditionalBreakpoints: false,
            supportsLogPoints: false,
            supportsTerminateRequest: true
        };
        this.sendResponse(response);
        this.sendEvent(new InitializedEvent());
    }

    protected async launchRequest(
        response: DebugProtocol.LaunchResponse,
        args: PonyppLaunchArgs
    ): Promise<void> {
        // 连接到 ponyppc DAP 模式
        const port = 4711;  // DAP 默认端口
        this._connection = net.createConnection({ port }, () => {
            this.sendDapRequest('initialize', {});
        });

        this._connection.on('data', (data: Buffer) => {
            this.handleDapMessage(data.toString());
        });

        this._connection.on('error', (err: Error) => {
            this.sendEvent(new TerminatedEvent());
        });

        this.sendDapRequest('launch', {
            program: args.program,
            args: args.args || [],
            stopOnEntry: args.stopOnEntry !== false
        });

        this.sendResponse(response);
    }

    protected setBreakPointsRequest(
        response: DebugProtocol.SetBreakpointsResponse,
        args: DebugProtocol.SetBreakpointsArguments
    ): void {
        const source = args.source.path || '';
        const breakpoints = (args.breakpoints || []).map(bp => {
            this.sendDapRequest('setBreakpoint', {
                file: source,
                line: bp.line,
                condition: bp.condition
            });
            return new Breakpoint(true, bp.line);
        });

        this._breakpoints.set(source, breakpoints);
        response.body = { breakpoints };
        this.sendResponse(response);
    }

    protected configurationDoneRequest(
        response: DebugProtocol.ConfigurationDoneResponse
    ): void {
        this.sendDapRequest('configurationDone', {});
        this.sendResponse(response);
    }

    protected continueRequest(
        response: DebugProtocol.ContinueResponse,
        args: DebugProtocol.ContinueArguments
    ): void {
        this.sendDapRequest('continue', { threadId: args.threadId });
        response.body = { allThreadsContinued: true };
        this.sendResponse(response);
        this.sendEvent(new ContinuedEvent(args.threadId, true));
    }

    protected nextRequest(
        response: DebugProtocol.NextResponse,
        args: DebugProtocol.NextArguments
    ): void {
        this.sendDapRequest('next', { threadId: args.threadId });
        this.sendResponse(response);
    }

    protected stepInRequest(
        response: DebugProtocol.StepInResponse,
        args: DebugProtocol.StepInArguments
    ): void {
        this.sendDapRequest('stepIn', { threadId: args.threadId });
        this.sendResponse(response);
    }

    protected stepOutRequest(
        response: DebugProtocol.StepOutResponse,
        args: DebugProtocol.StepOutArguments
    ): void {
        this.sendDapRequest('stepOut', { threadId: args.threadId });
        this.sendResponse(response);
    }

    protected threadsRequest(
        response: DebugProtocol.ThreadsResponse
    ): void {
        const threads: Thread[] = [new Thread(0, 'Main')];
        for (const [id, name] of this._actorThreads) {
            threads.push(new Thread(id, `Actor: ${name}`));
        }
        response.body = { threads };
        this.sendResponse(response);
    }

    protected stackTraceRequest(
        response: DebugProtocol.StackTraceResponse,
        args: DebugProtocol.StackTraceArguments
    ): void {
        this.sendDapRequest('stackTrace', { threadId: args.threadId });
        // 响应通过 handleDapMessage 异步处理
        this._pendingResponses.set('stackTrace', response);
    }

    protected scopesRequest(
        response: DebugProtocol.ScopesResponse,
        args: DebugProtocol.ScopesArguments
    ): void {
        const scope = new Scope('Locals', this.createHandle({ type: 'locals' }), false);
        response.body = { scopes: [scope] };
        this.sendResponse(response);
    }

    protected variablesRequest(
        response: DebugProtocol.VariablesResponse,
        args: DebugProtocol.VariablesArguments
    ): void {
        this.sendDapRequest('variables', {
            variablesReference: args.variablesReference
        });
        this._pendingResponses.set('variables', response);
    }

    protected terminateRequest(
        response: DebugProtocol.TerminateResponse,
        args: DebugProtocol.TerminateArguments
    ): void {
        this.sendDapRequest('terminate', {});
        this._connection?.destroy();
        this.sendResponse(response);
    }

    // ==================== DAP 通信 ====================

    private _pendingResponses: Map<string, DebugProtocol.Response> = new Map();

    private sendDapRequest(command: string, args: any): void {
        if (!this._connection) return;
        const msg = JSON.stringify({ command, args });
        const header = `Content-Length: ${Buffer.byteLength(msg)}\r\n\r\n`;
        this._connection.write(header + msg);
    }

    private handleDapMessage(data: string): void {
        // 解析 DAP 消息
        const headerEnd = data.indexOf('\r\n\r\n');
        if (headerEnd === -1) return;

        const header = data.substring(0, headerEnd);
        const contentLength = parseInt(
            header.match(/Content-Length: (\d+)/)?.[1] || '0'
        );
        const body = data.substring(headerEnd + 4, headerEnd + 4 + contentLength);

        try {
            const msg = JSON.parse(body);
            this.handleDapEvent(msg);
        } catch (e) {
            // 忽略解析错误
        }
    }

    private handleDapEvent(msg: any): void {
        switch (msg.type) {
            case 'event':
                switch (msg.event) {
                    case 'stopped':
                        this.sendEvent(new StoppedEvent(
                            msg.body.reason || 'step',
                            msg.body.threadId || 0
                        ));
                        break;
                    case 'terminated':
                        this.sendEvent(new TerminatedEvent());
                        break;
                    case 'actorStarted':
                        this._actorThreads.set(msg.body.id, msg.body.name);
                        break;
                }
                break;
            case 'response':
                const pending = this._pendingResponses.get(msg.command);
                if (pending) {
                    pending.body = msg.body;
                    this.sendResponse(pending);
                    this._pendingResponses.delete(msg.command);
                }
                break;
        }
    }

    private createHandle(data: any): number {
        const id = this._nextHandleId++;
        this._variableHandles.set(id, data);
        return id;
    }
}

// 启动
DebugSession.run(PonyppDebugSession);
