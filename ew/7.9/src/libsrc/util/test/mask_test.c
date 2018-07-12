
#include <stdio.h>

char ip_in_same_netmask(char * ip1, char * ip2, char *netmask);


int main() {

char ip1[] = "172.16.11.5";
char ip_to_test[] = "172.16.12.6";
char netmask_to_succeed[] = "255.255.0.0";
char netmask_to_fail[] = "255.255.255.0";
char netmask_match_any[] = "0.0.0.0";
char netmask_match_perfect[] = "255.255.255.255";

if (ip_in_same_netmask(ip1, ip_to_test, netmask_to_succeed)) fprintf(stdout, "algo works for match\n");
if (!ip_in_same_netmask(ip1, ip_to_test, netmask_to_fail)) fprintf(stdout, "algo works for fail\n");
if (ip_in_same_netmask(ip1, ip_to_test, netmask_match_any)) fprintf(stdout, "algo works for any-match %s\n", netmask_match_any);
if (!ip_in_same_netmask(ip1, ip_to_test, netmask_match_perfect)) fprintf(stdout, "algo works for perfect-match %s fail case\n", netmask_match_perfect);
if (ip_in_same_netmask(ip1, ip1, netmask_match_perfect)) fprintf(stdout, "algo works for perfect-match %s succeed case\n", netmask_match_perfect);
}
