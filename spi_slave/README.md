# Hardware diagram

```ditaa

                 ^
                 |
                +++
                | |0.6MΩ Pull-up
                +++
                 |
+------------+   |   +------------+
|            |   +---+INT         |
|            |       |            |
|          CS+------>+CS          |
|            |       |            |
|  SPI    CLK+------>+CLK SPI     |
|  MASTER    |       |    SLAVE   |
|         DAT+-------+DAT         |
|            |       |            |
|         INT+<------+DRY         |
|            |       |            |
|            |       |            |
+------------+       +------------+


```
