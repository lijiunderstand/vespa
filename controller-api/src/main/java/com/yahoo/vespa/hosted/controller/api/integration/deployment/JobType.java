// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.controller.api.integration.deployment;

import com.yahoo.config.provision.Environment;
import com.yahoo.config.provision.RegionName;
import com.yahoo.config.provision.zone.ZoneId;
import com.yahoo.vespa.hosted.controller.api.integration.zone.ZoneRegistry;

import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static ai.vespa.validation.Validation.require;
import static com.yahoo.config.provision.Environment.dev;
import static com.yahoo.config.provision.Environment.perf;
import static com.yahoo.config.provision.Environment.prod;
import static com.yahoo.config.provision.Environment.staging;
import static com.yahoo.config.provision.Environment.test;
import static java.util.Comparator.naturalOrder;
import static java.util.stream.Collectors.toUnmodifiableList;

/**
 * Specification for a deployment and/or test job to run: what zone, and whether it is a production test.
 *
 * @author jonmv
 */
public final class JobType implements Comparable<JobType> {

    private final String jobName;
    private final ZoneId zone;
    private final boolean isProductionTest;

    private JobType(String jobName, ZoneId zone, boolean isProductionTest) {
        this.jobName = jobName;
        this.zone = zone;
        this.isProductionTest = isProductionTest;
    }

    /** A system test in a test zone, or throws if no test zones are present.. */
    public static JobType systemTest(ZoneRegistry zones) {
        return testIn(test, zones);
    }

    /** A staging test in a staging zone, or throws if no staging zones are present. */
    public static JobType stagingTest(ZoneRegistry zones){
        return testIn(staging, zones);
    }

    private static JobType testIn(Environment environment, ZoneRegistry zones) {
        return zones.zones().controllerUpgraded().in(environment).zones().stream().map(zone -> deploymentTo(zone.getId()))
                    .findFirst().orElseThrow(() -> new IllegalArgumentException("no zones in " + environment + " among " + zones.zones().controllerUpgraded().zones()));
    }

    /** A deployment to the given dev region. */
    public static JobType dev(RegionName region) {
        return deploymentTo(ZoneId.from(dev, region));
    }

    /** A deployment to the given dev region. */
    public static JobType dev(String region) {
        return deploymentTo(ZoneId.from("dev", region));
    }

    /** A deployment to the given perf region. */
    public static JobType perf(RegionName region) {
        return deploymentTo(ZoneId.from(perf, region));
    }

    /** A deployment to the given perf region. */
    public static JobType perf(String region) {
        return deploymentTo(ZoneId.from("perf", region));
    }

    /** A deployment to the given prod region. */
    public static JobType prod(RegionName region) {
        return deploymentTo(ZoneId.from(prod, region));
    }

    /** A deployment to the given prod region. */
    public static JobType prod(String region) {
        return deploymentTo(ZoneId.from("prod", region));
    }

    /** A production test in the given region. */
    public static JobType test(RegionName region) {
        return productionTestOf(ZoneId.from(prod, region));
    }

    /** A production test in the given region. */
    public static JobType test(String region) {
        return productionTestOf(ZoneId.from("prod", region));
    }

    /** A deployment to the given zone; this may be a zone in the {@code test} or {@code staging} environments. */
    public static JobType deploymentTo(ZoneId zone) {
        String name;
        switch (zone.environment()) {
            case prod: name = "production-" + zone.region().value(); break;
            case test: name = "system-test"; break;
            case staging: name = "staging-test"; break;
            default: name = zone.environment().value() + "-" + zone.region().value();
        }
        return new JobType(name, zone, false);
    }

    /** A production test in the given production zone. */
    public static JobType productionTestOf(ZoneId zone) {
        String name = "test-" + require(zone.environment() == prod, zone, "must be prod zone").region().value();
        return new JobType(name, zone, true);
    }

    /** Creates a new job type from serialized zone data, and whether it is a production test; the inverse of {@link #serialized()} */
    public static JobType ofSerialized(String raw) {
        String[] parts = raw.split("\\.");
        if (parts.length == 2) return deploymentTo(ZoneId.from(parts[0], parts[1]));
        if (parts.length == 3 && "test".equals(parts[2])) return productionTestOf(ZoneId.from(parts[0], parts[1]));
        throw new IllegalArgumentException("illegal serialized job type '" + raw + "'");
    }

    /** Creates a new job type from a job name, and a zone registry for looking up zones for the special system and staging test types. */
    public static JobType fromJobName(String jobName, ZoneRegistry zones) {
        String[] parts = jobName.split("-", 2);
        if (parts.length != 2) throw new IllegalArgumentException("job names must be 'system-test', 'staging-test', or environment and region parts, separated by '-', but got: " + jobName);
        switch (parts[0]) {
            case "system": return systemTest(zones);
            case "staging": return stagingTest(zones);
            case "production": return prod(parts[1]);
            case "test": return test(parts[1]);
            case "dev": return dev(parts[1]);
            case "perf": return perf(parts[1]);
            default: throw new IllegalArgumentException("job names must begin with one of: system, staging, production, test, dev, perf; but got: " + jobName);
        }
    }

    public static List<JobType> allIn(ZoneRegistry zones) {
        return zones.zones().controllerUpgraded().zones().stream()
                    .flatMap(zone -> zone.getEnvironment().isProduction() ? Stream.of(deploymentTo(zone.getId()), productionTestOf(zone.getId()))
                                                                          : Stream.of(deploymentTo(zone.getId())))
                    .sorted(naturalOrder())
                    .collect(toUnmodifiableList());
    }

    /** A serialized form of this: {@code &lt;environment&gt;.&lt;region&gt;[.test]}; the inverse of {@link #ofSerialized(String)} */
    public String serialized() {
        return zone.environment().value() + "." + zone.region().value() + (isProductionTest ? ".test" : "");
    }

    public String jobName() {
        return jobName;
    }

    /** Returns the zone for this job. */
    public ZoneId zone() {
        return zone;
    }

    public boolean isSystemTest() {
        return environment() == test;
    }

    public boolean isStagingTest() {
        return environment() == staging;
    }

    /** Returns whether this is a production job */
    public boolean isProduction() {
        return environment() == prod;
    }

    /** Returns whether this job runs tests */
    public boolean isTest() {
        return isProductionTest || environment().isTest();
    }

    /** Returns whether this job deploys to a zone */
    public boolean isDeployment() {
        return ! isProductionTest;
    }

    /** Returns the environment of this job type */
    public Environment environment() {
        return zone.environment();
    }

    @Override
    public int compareTo(JobType other) {
        int result;
        if (0 != (result = environment().compareTo(other.environment()))) return -result;
        if (0 != (result = zone.region().compareTo(other.zone.region()))) return result;
        return Boolean.compare(isProductionTest, other.isProductionTest);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        JobType jobType = (JobType) o;
        return jobName.equals(jobType.jobName);
    }

    @Override
    public int hashCode() {
        return jobName.hashCode();
    }

    @Override
    public String toString() {
        return jobName;
    }

}
