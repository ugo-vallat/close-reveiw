const std = @import("std");

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) void {
    const c_include_list = &.{
        "network/chat.c",
        "network/p2p-com.c",
        "network/manager.c",
        "network/tls-com.c",
        "types/command.c",
        "types/list.c",
        "types/genericlist.c",
        "types/message.c",
        "types/p2p-msg.c",
        "types/packet.c",
        "utils/config.c",
        "utils/logger.c",
        // "utils/token.c",
    };
    const c_include_list_server = &.{
        "database-manager.c",
        // "passlist_to_hashlist.c",
        "weak_password.c",
        "request-handler.c",
    };
    const c_include_list_client = &.{
        "tui.c",
        // "history-manager.c",
    };
    const flags = &.{
        "-g",
        "-pedantic",
        "-rdynamic",
        "-Wextra",
        "-Wall",
        "-pthread",
        // "-std=c99",
        "-D__USE_UNIX98",
        "-D__USE_XOPEN2K",
        "-DDEBUG",
    };

    const server = b.addExecutable(.{
        .name = "close-review-server",
        .link_libc = true,
        .target = b.graph.host,
    });
    server.addCSourceFile(.{ .file = .{ .path = "src/server/main.c" }, .flags = flags });
    server.addIncludePath(.{ .path = "include/" });
    server.linkSystemLibrary2("openssl", .{ .use_pkg_config = .force });
    server.linkSystemLibrary2("mariadb", .{ .use_pkg_config = .force });
    server.addCSourceFiles(.{ .root = .{ .path = "src/" }, .files = c_include_list, .flags = flags });
    server.addCSourceFiles(.{ .root = .{ .path = "src/server/" }, .files = c_include_list_server, .flags = flags });

    const client = b.addExecutable(.{
        .name = "close-review-client",
        .link_libc = true,
        .target = b.graph.host,
    });
    client.addCSourceFile(.{ .file = .{ .path = "src/client/main.c" }, .flags = flags });
    client.addIncludePath(.{ .path = "include/" });
    client.linkSystemLibrary2("openssl", .{ .use_pkg_config = .force });
    client.linkSystemLibrary2("ncurses", .{ .use_pkg_config = .force });
    client.addCSourceFiles(.{ .root = .{ .path = "src/" }, .files = c_include_list, .flags = flags });
    client.addCSourceFiles(.{ .root = .{ .path = "src/client/" }, .files = c_include_list_client, .flags = flags });
    b.installArtifact(server);
    b.installArtifact(client);

    const server_run_cmd = b.addRunArtifact(server);
    const client_run_cmd = b.addRunArtifact(client);

    server_run_cmd.step.dependOn(b.getInstallStep());
    client_run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        server_run_cmd.addArgs(args);
    }

    if (b.args) |args| {
        client_run_cmd.addArgs(args);
    }

    const server_run_step = b.step("server-run", "Run the Server CLI");
    server_run_step.dependOn(&server_run_cmd.step);

    const client_run_step = b.step("client-run", "Run the Client CLI");
    client_run_step.dependOn(&client_run_cmd.step);
}
