// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.jdisc.application;

import com.yahoo.jdisc.NoopSharedResource;
import com.yahoo.jdisc.service.ServerProvider;
import org.junit.Test;

import java.util.Arrays;
import java.util.Iterator;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

/**
 * @author Simon Thoresen Hult
 */
public class ServerRepositoryTestCase {

    @Test
    public void requireThatInstallWorks() {
        ServerRepository servers = newServerRepository();
        MyServer server = new MyServer();
        servers.install(server);

        Iterator<ServerProvider> it = servers.iterator();
        assertTrue(it.hasNext());
        assertSame(server, it.next());
        assertFalse(it.hasNext());
    }

    @Test
    public void requireThatInstallAllWorks() {
        ServerRepository servers = newServerRepository();
        ServerProvider foo = new MyServer();
        ServerProvider bar = new MyServer();
        servers.installAll(Arrays.asList(foo, bar));

        Iterator<ServerProvider> it = servers.iterator();
        assertTrue(it.hasNext());
        assertSame(foo, it.next());
        assertTrue(it.hasNext());
        assertSame(bar, it.next());
        assertFalse(it.hasNext());
    }

    @Test
    public void requireThatUninstallWorks() {
        ServerRepository servers = newServerRepository();
        ServerProvider server = new MyServer();
        servers.install(server);
        servers.uninstall(server);
        assertFalse(servers.iterator().hasNext());
    }

    @Test
    public void requireThatUninstallAllWorks() {
        ServerRepository servers = newServerRepository();
        ServerProvider foo = new MyServer();
        ServerProvider bar = new MyServer();
        ServerProvider baz = new MyServer();
        servers.installAll(Arrays.asList(foo, bar, baz));
        servers.uninstallAll(Arrays.asList(foo, bar));
        Iterator<ServerProvider> it = servers.iterator();
        assertNotNull(it);
        assertTrue(it.hasNext());
        assertSame(baz, it.next());
        assertFalse(it.hasNext());
    }

    private static ServerRepository newServerRepository() {
        return new ServerRepository(new GuiceRepository());
    }

    private static class MyServer extends NoopSharedResource implements ServerProvider {

        @Override
        public void start() {

        }

        @Override
        public void close() {

        }
    }
}
