// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.grouping.vespa;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * @author Simon Thoresen Hult
 */
public class ResultIdTestCase {

    @Test
    public void requireThatStartsWithWorks() {
        assertFalse(ResultId.valueOf().startsWith(1, 1, 2, 3));
        assertFalse(ResultId.valueOf(1).startsWith(1, 1, 2, 3));
        assertFalse(ResultId.valueOf(1, 1).startsWith(1, 1, 2, 3));
        assertFalse(ResultId.valueOf(1, 1, 2).startsWith(1, 1, 2, 3));
        assertTrue(ResultId.valueOf(1, 1, 2, 3).startsWith(1, 1, 2, 3));
        assertTrue(ResultId.valueOf(1, 1, 2, 3).startsWith(1, 1, 2));
        assertTrue(ResultId.valueOf(1, 1, 2, 3).startsWith(1, 1));
        assertTrue(ResultId.valueOf(1, 1, 2, 3).startsWith(1));
        assertTrue(ResultId.valueOf(1, 1, 2, 3).startsWith());
    }

    @Test
    public void requireThatChildIdStartsWithParentId() {
        ResultId parentId = ResultId.valueOf(1, 1, 2);
        ResultId childId = parentId.newChildId(3);
        assertTrue(childId.startsWith(1, 1, 2));
    }

    @Test
    public void requireThatHashCodeIsImplemented() {
        assertEquals(ResultId.valueOf(1, 1, 2, 3).hashCode(), ResultId.valueOf(1, 1, 2, 3).hashCode());
        assertFalse(ResultId.valueOf(1, 1, 2, 3).hashCode() == ResultId.valueOf(5, 8, 13, 21).hashCode());
    }

    @Test
    public void requireThatEqualsIsImplemented() {
        assertEquals(ResultId.valueOf(1, 1, 2, 3), ResultId.valueOf(1, 1, 2, 3));
        assertFalse(ResultId.valueOf(1, 1, 2, 3).equals(ResultId.valueOf(5, 8, 13, 21)));
    }

    @Test
    public void requireThatResultIdCanBeEncoded() {
        assertEncode("BIBCBCBEBG", ResultId.valueOf(1, 1, 2, 3));
        assertEncode("BIBKCBACBKCCK", ResultId.valueOf(5, 8, 13, 21));
    }

    @Test
    public void requireThatResultIdCanBeDecoded() {
        assertDecode(ResultId.valueOf(1, 1, 2, 3), "BIBCBCBEBG");
        assertDecode(ResultId.valueOf(5, 8, 13, 21), "BIBKCBACBKCCK");
    }

    private static void assertEncode(String expected, ResultId toEncode) {
        IntegerEncoder encoder = new IntegerEncoder();
        toEncode.encode(encoder);
        assertEquals(expected, encoder.toString());
    }

    private static void assertDecode(ResultId expected, String toDecode) {
        IntegerDecoder decoder = new IntegerDecoder(toDecode);
        assertTrue(decoder.hasNext());
        assertEquals(expected, ResultId.decode(decoder));
        assertFalse(decoder.hasNext());
    }
}
