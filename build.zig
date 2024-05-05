const std = @import("std");

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) void {
    const c_include_list = &[_][]const u8{
        "placeholder.c",
    };
    const c_include_list_server = &[_][]const u8{
        "placeholder.c",
    };
    const c_include_list_client = &[_][]const u8{
        "placeholder.c",
    };

    const server = b.addExecutable(.{
        .name = "close-review-server",
        .link_libc = true,
        .target = b.host,
    });
    server.addCSourceFile(.{ .file = .{ .path = "src/server/main.c" } });
    server.addIncludePath(.{ .path = "include/" });
    server.addCSourceFiles(.{ .root = .{ .path = "src/" }, .files = c_include_list });
    server.addCSourceFiles(.{ .root = .{ .path = "src/server/" }, .files = c_include_list_server });

    const client = b.addExecutable(.{
        .name = "close-review-client",
        .link_libc = true,
        .target = b.host,
    });
    client.addCSourceFile(.{ .file = .{ .path = "src/client/main.c" } });
    client.addIncludePath(.{ .path = "include/" });
    client.addCSourceFiles(.{ .root = .{ .path = "src/" }, .files = c_include_list });
    client.addCSourceFiles(.{ .root = .{ .path = "src/client/" }, .files = c_include_list_client });

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
    client_run_step.dependOn(&server_run_cmd.step);
}
