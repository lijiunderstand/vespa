// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespavisit;

import org.junit.Test;
import static org.junit.Assert.*;

public class VdsVisitTargetTestCase {

    @Test
    public void testParametersSlobrok() throws Exception {
        VdsVisitTarget target = new VdsVisitTarget();
        target.parseArguments(new String[]{
                "--bindtoslobrok", "myname",
                "--processtime", "34",
                "--visithandler", "Foo",
                "--visitoptions", "foo bar zoo",
                "-i",
                "-v"
        });

        assertEquals("myname", target.getSlobrokAddress());
        assertEquals(34, target.getProcessTime());
        assertEquals("Foo", target.getHandlerClassName());
        assertEquals(3, target.getHandlerArgs().length);
        assertEquals("foo", target.getHandlerArgs()[0]);
        assertEquals("bar", target.getHandlerArgs()[1]);
        assertEquals("zoo", target.getHandlerArgs()[2]);
        assertTrue(target.isVerbose());
        assertTrue(target.isPrintIds());
    }

    @Test
    public void testParametersPort() throws Exception {
        VdsVisitTarget target = new VdsVisitTarget();
        target.parseArguments("--bindtosocket 1234".split(" "));
        assertEquals(1234, target.getPort());
        assertEquals(null, target.getSlobrokAddress());
    }

    public void assertException(String params) {
        try {
            VdsVisitTarget target = new VdsVisitTarget();
            target.parseArguments(params.split(" "));
            assertTrue(false);
        } catch (Exception e) {

        }
    }

    @Test
    public void testPortAndSlobrok() {
        assertException("--bindtoslobrok foo --bindtosocket 1234");
        assertException("--bindtoport foo");
    }

}
